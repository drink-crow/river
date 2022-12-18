river 是一个使用C++编写的，可以处理曲线的多边形裁剪库。多边形裁剪是各种工业、设计软件常见功能，但是常见的多边形裁剪算法基本是建立在纯线段的基础上。在实际应用中，常需要对包含曲线内容的多边形进行计算。在作者的实际工作经验和见闻中，许多公司（包括一些大公司），都十分需要这方面的内容。常见的各种多边形裁剪库，诸如clipper，尽管其本身十分优秀，缺不支持对曲线的运算。支持曲线运算的，如 CGAL 中的功能库，其算法库体积庞大且代码复杂，使用上也有诸多限制。
对多边形进行裁剪并不是一件特别简单的事情，而且根据实际经验表明，这项操作对运算的速度有特别高的要求。例如对文字进行轮廓生成，其中重要的一步就是对生成多边形进行，在该操作中，往往需要计算海量的数据。只要稍微提高一下细分程度，每个字符的多边形轮廓就可以有上千个节点，一段文字的所有节点接起来可以轻松到达数十万。

参考
 Max K. Agoston. Computer Graphics and Geometric Modelling:Implementation & Algorithms[M]  v.1-Springer, 2005, page:84-109

这里介绍了几种〇五年以前常见的 polygon clipping 算法。分别是 

Sutherland-Hodgman Polygon Clipping
Weiler Polygon Clipping
Liang-Barsky Polygon Clipping
Maillot Polygon Clipping
Vatti Polygon Clipping
Greiner-Hormann Polygon Clipping

按照书籍中给出的评价，前4种算法不再深入，主要集中在 Vatti 和 Greiner-Hormann 两种算法。Greiner-Hormann 是基于 Weiler Polygon Clipping 改良的，算法简单，速度快，稳定性好。主要的缺点是，仅给出了裁剪操作的外轮廓，对于内含空洞的情况，需要自行判断包含关系和环绕数。
Vatti 法的兼容性最高，而且有已经验证的案例，开源算法库 Clipper 便是基于此算法。对于自交、孔洞均能很好的处理。但是该算法比较复杂，需要注意的细节很多。

