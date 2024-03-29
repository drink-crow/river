# RIVER

## 简介 / Introduction

river 是一个使用C++编写的，可以处理曲线的多边形裁剪库。  

目前 river 支持的线段类型有直线、三阶贝塞尔  

![preview img](./river_test.svg "效果示意 / Preview")

## 编译 / Build

### 常规编译 / Normal Build

适合仅想轻松使用该库的开发者。将构建 river 静态链接库

环境 / environment:  

+ boost 1.80.0  
  + boost.spirit.qi
  + boost.pool

### 开发编译 / Develop Build

适合想了解、或调试该库的开发者，还将额外编译一些用于辅助调试的信息和目标

要求 / requierd:  

+ 常规编译的环境 / Normal Build`s environment  
+ Qt 6.4.1  
  + Gui
  + Widgets
+ libcurl 7.86.0
+ drogon 1.8.2

## 使用 / Usage

```cpp
  river::processor river;
  auto subject = river::make_path(
    "m 18 15.2 l 5.1 15.2 c 4 15.2 3 15 2.3 14.6 c 1.7 14.2 0.9 13.4 0 12 l 1 11 c 1.5 12 2 12.5 2.5 12.7 c 3 13 3.5 13 4.3 13 l 18 13");
  auto clip = river::make_path(
    "m 5 13 l 5 11 c 5 6.8 4.5 3.5 4 1 l 4 0 l 7 0 l7 13"
    "m 12 13 l 12 4 c 12 2.5 12 1.5 12.5 1 c 13 0.3 13.6 0 14.5 0 c 15.6 0 16.7 0.5 18 1.5 l 17.3 2.4 c 16.6 2 16 1.7 15.7 1.7 c 15 1.6 14.5 2.3 14.5 3.7 l 14.5 13");

  river.add_path(subject, path_type::subject);
  river.add_path(clip, path_type::clip);
  river::paths solution;
  river.process(clip_type::union_, fill_rule::positive, solution);
```

多边形裁剪是各种工业、设计软件常见功能，但是常见的多边形裁剪算法基本是建立在纯线段的基础上。在实际应用中，常需要对包含曲线内容的多边形进行计算。在作者的实际工作经验和见闻中，许多公司（包括一些大公司），都十分需要这方面的内容。常见的各种多边形裁剪库，诸如clipper，尽管其本身十分优秀，缺不支持对曲线的运算。支持曲线运算的，如 CGAL 中的功能库，其算法库体积庞大且代码复杂，使用上也有诸多限制。  

对多边形进行裁剪并不是一件特别简单的事情，而且根据实际经验表明，这项操作对运算的速度有特别高的要求。例如对文字进行轮廓生成，其中重要的一步就是对生成多边形进行，在该操作中，往往需要计算海量的数据。只要稍微提高一下细分程度，每个字符的多边形轮廓就可以有上千个节点，一段文字的所有节点接起来可以轻松到达数十万。

参考 Max K. Agoston. Computer Graphics and Geometric Modelling:Implementation & Algorithms[M]  v.1-Springer, 2005, page:84-109

这里介绍了几种〇五年以前常见的 polygon clipping 算法。分别是  

+ Sutherland-Hodgman Polygon Clipping
+ Weiler Polygon Clipping
+ Liang-Barsky Polygon Clipping
+ Maillot Polygon Clipping
+ Vatti Polygon Clipping
+ Greiner-Hormann Polygon Clipping

按照书籍中给出的评价，前4种算法不再深入，主要集中在 Vatti 和 Greiner-Hormann 两种算法。Greiner-Hormann 是基于 Weiler Polygon Clipping 改良的，算法简单，速度快，稳定性好。主要的缺点是，仅给出了裁剪操作的外轮廓，对于内含空洞的情况，需要自行判断包含关系和环绕数。  
Vatti 法的兼容性最高，而且有已经验证的案例，开源算法库 Clipper 便是基于此算法。对于自交、孔洞均能很好的处理。但是该算法比较复杂，需要注意的细节很多。得益于类似扫描线的处理方法，此方法可以快速得出轮廓的包含关系和环绕数，因此能处理与、或、亦或等操作，也可以以多种环绕数规则定义区域，甚至可以扩展到对三位空间区域进行裁剪。该法中最大的难点在于水平线的处理。另一方面，该法在扫描的过程中同时处理线段相交的情况和多边形的构造，使得该过程的描述特别复杂。  
想要对 Vatti 进行拓展以兼容曲线，需要将该算法中一些直接使用到线段性质的地方修改。例如在线段组成的多边形中，局部最低点永远位于节点上，而曲线并非如此。或者处理掉一些特殊情况后，线段和线段只有一个交点。最为重要的一个地方，原算法中可以直接使用顶点位置来记录一个多边形，包括直接使用一个节点来记录求交的结果，显然包含曲线的内容是不能这么做的。因此原本在扫描过程中进行的交点处理，必须提前进行，也可以使得算法的结构更加简单明了，但是会降低效率。其实曲线求交的过程仍可放入原始的扫描过程中，但是反复对曲线进行求交判断，会额外增加大量的运算，各种特殊情况也会大大增加流程的复杂度——对线段间的求交判断则简单得多，自然没有这种问题。同时亦要对输出的曲线进行拆分，使拆分出来的每一部分在y轴上是单调递增的。  
总结一下，有一次啊几点是异于原始算法的：  

算法流程

``` cpp
y = first_scanline_y
while(true)
{
    insert_local_minima(y)
    /* local minima 的左右两条边都需要插入到 ael 正确的位置，ael 以 curr_x 排序，相同 ael 时，顺时针排序（从左到右），完全相同的需要标注出来，还需要注意兼容曲线内容
    标记环绕数受到影响的点位
    根据标记的点位重新计算受到影响的 wind_cnt、wind_cnt2
    然后根据 cliptype 和 fill rule 还有 path_type 等识别出新的需要输出的区域的左右轮廓 */
    pop_scanline(y)
    do_top_of_scanbeam(y)
        从左到右遍历 ael：
            没有到达 scanline 没到 top 的 ae
                continue;
            到达 top 的 ae
                如果有 out_path, 添加它的 seg 到 out_path 对应的位置（起点或末端）
                ae 更新到下一条边
                如果该 ae 没有和别的点相交
                    continue;
                现在 ae 和别的线（不考虑自身的下一条线）相交了，会对 out_path 接下来的走向产生影响
                标记需要重算的点位，因为接下来的有可能发生 intersection 和点位
                如果该 ae 和输出有关系，则代表输出的区域对应的一侧失去了 ae，需要在下一轮选择新的边界或者和另一侧的合并
}
```

重合线段的处理。关键的点是 calc_windcnt() 的计算以及 is_contributing() 的判断。首先 is_contributing() 的判断只和 wind_cnt 和 wind_cnt2 有关，所以考虑范围可以进一步缩小到 calc_windcnt() 计算上。重合的线段，环绕数应当统一处理，不然会产生轮廓相贴的区域，产生了后续再判断和处理也挺麻烦的。但是 is_contributing() 无法判断产生的区域是否是闭合的。因此统一处理的办法不行。（范例————两个相贴的外轮廓矩形，中间的垂直线的环绕数从左往右是1，从右往左也是1，但是 is_contributing() 中默认环绕数左右是相差1的。  
因此重合线段视为普通线段处理，但是在计算输出边缘时，会判断是否和左边的输出重合，重合则相互抵消  
