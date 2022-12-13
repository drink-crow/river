#include "debug_util.h"

namespace debug_util {
    QGraphicsScene* debug_scene = nullptr;

    QGraphicsScene* get_debug_scene()
    {
        if(debug_scene == nullptr){
            debug_scene =  new QGraphicsScene;
        }
        return debug_scene;
    }
}
