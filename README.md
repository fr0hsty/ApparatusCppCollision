# ApparatusCppCollision
An attempt to bypass UE4's components and talk directly to PhysX..

There are many hacks that are possible inorder to enjoin ECS and Physics, and a lot of them will work without issue.

This repo attempts to enjoin directly PhysX Colliders, and Apparatus FSubjectHandles such that a user can always communicate directly from FSubjectHandles -> to PhysX colliders, and vice versa - to be able to, directly from an overlap or raytrace, retreive that colliders relevant FSubjectHandle.
