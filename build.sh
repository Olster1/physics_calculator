clear
clear
CXX=clang++

RELEASE="-DNDEBUG -O2"
DEBUG="-g"
WARNINGS="-Wno-deprecated-declarations -Wno-writable-strings -Wno-c++11-compat-deprecated-writable-strings -Wno-tautological-compare"

SDL_FRAMEWORK_SLICE="/Library/Frameworks/SDL3.xcframework/macos-arm64_x86_64"
SDL_IMAGE_FRAMEWORK_SLICE="/Library/Frameworks/SDL3_image.xcframework/macos-arm64_x86_64"

# Ensure output directory exists
mkdir -p ./bin

$CXX ./platform_backends/sdl_platform_layer.cpp \
  -std=c++11 \
  $DEBUG \
  -arch arm64 -arch x86_64 \
  $WARNINGS \
  -iframework "$SDL_FRAMEWORK_SLICE" \
  -iframework "$SDL_IMAGE_FRAMEWORK_SLICE" \
  -F "$SDL_FRAMEWORK_SLICE" \
  -F "$SDL_IMAGE_FRAMEWORK_SLICE" \
  -framework SDL3 \
  -framework SDL3_image \
  -Wl,-rpath,@executable_path/../Frameworks \
  -Wl,-rpath,"$SDL_FRAMEWORK_SLICE" \
  -Wl,-rpath,"$SDL_IMAGE_FRAMEWORK_SLICE" \
  -o ./bin/KenKen
