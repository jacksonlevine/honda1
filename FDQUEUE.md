







OPTIMIZATION IDEA:

only rebuild the chunks that weren't previously in range. 






OPTIMIZATION IDEA:
Initiative: Reduce size of information buffered for geometry

- STORE CUBE FACE VERTICES ON VERTEX SHADER, AND PASS BYTE INDEXES



Only visit each voxel once.

Idea: recursive building of chunk mesh expanding out from one spot













                                                                                                                                DONE                                    REALTIME NOISE (POSSIBLE OPTIMIZATION):
                                                                                                                                                                                get terrain from realtime noise funcs, store changed only


SHELL GROUND:
    Refactor heightmap to have a HeightTile struct {    bool broken          }


Fill all space below heightmap with blocks



Raycast ---> hit block -----> break
|
|
v
hit heightmap
set bool broken


Don't render if broken 