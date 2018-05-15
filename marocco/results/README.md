# Build Instructions

## Build boost libraries for emscripten

```bash
# build zlib for boost's iostreams library
singularity exec CONTAINER.IMG /path/to/embuilder.py build zlib

# and download boost sources, extract
tar xfj boost_1_66_0.tar.bz2

cd boost_1_66_0/

# configure with default gcc flow
singularity exec --app visionary-defaults CONTAINER.IMG ./bootstrap.sh

# enable emscripten (+ previously built zlib)
sed -i 's,using gcc ;,using emscripten : : em++ -s USE_ZLIB=1 ;,' project-config.jam
sed -i 's,project : default-build <toolset>gcc ;,project : default-build <toolset>emscripten ;,' project-config.jam

# patch boost
cat <<EOF > fix-emscripten-boost-1.66.0.patch
diff -pur boost_1_66_0_new/tools/build/src/tools/emscripten.jam boost_1_66_0/tools/build/src/tools/emscripten.jam
--- boost_1_66_0_new/tools/build/src/tools/emscripten.jam	2017-12-14 00:56:50.000000000 +0100
+++ boost_1_66_0/tools/build/src/tools/emscripten.jam	2018-05-15 11:11:51.238844201 +0200
@@ -7,6 +7,7 @@ import feature ;
 import os ;
 import toolset ;
 import common ;
+import generators ;
 import gcc ;
 import type ;
 
@@ -41,6 +42,9 @@ toolset.inherit-generators emscripten <t
     : gcc
     : gcc.mingw.link gcc.mingw.link.dll gcc.compile.c.pch gcc.compile.c++.pch
     ;
+
+generators.override emscripten.searched-lib-generator : searched-lib-generator ;
+
 toolset.inherit-rules emscripten : gcc ;
 toolset.inherit-flags emscripten : gcc 
         :
@@ -54,8 +58,9 @@ toolset.inherit-flags emscripten : gcc
         ;
 
 type.set-generated-target-suffix EXE : <toolset>emscripten : "js" ;
-type.set-generated-target-suffix OBJ : <toolset>emscripten : "bc" ;
-type.set-generated-target-suffix STATIC_LIB : <toolset>emscripten : "bc" ;
+type.set-generated-target-suffix OBJ : <toolset>emscripten : "o" ;
+type.set-generated-target-suffix SHARED_LIB : <toolset>emscripten : "o" ;
+type.set-generated-target-suffix STATIC_LIB : <toolset>emscripten : "a" ;
 
 toolset.flags emscripten.compile OPTIONS <flags> ;
 toolset.flags emscripten.compile OPTIONS <cflags> ;
EOF
patch -p1 < fix-emscripten-boost-1.66.0.patch


# emcc/clang binaries...
export SINGULARITYENV_PATH=$PATH:/path/to/EMSCRIPTEN_ROOT/in/container

singularity exec CONTAINER.IMG ./b2 -a variant=release link=static install --layout=system --with=serialization --prefix=$PWD/install
```

## Build marocco results using emscripten

```
singularity exec --app visionary-defaults CONTAINER.IMG ./waf configure build --targets=Marocco -j1 -v --emscripted-boost-prefix=/path/to/PWD/install/of/previous/boost/build
```
