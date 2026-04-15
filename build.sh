clear
clear
CXX=clang++

RELEASE="-DNDEBUG -O2"
DEBUG="-g"
WARNINGS="-Wno-deprecated-declarations -Wno-writable-strings -Wno-c++11-compat-deprecated-writable-strings -Wno-tautological-compare"

SDL_FRAMEWORK_SLICE="/Library/Frameworks/SDL3.xcframework/macos-arm64_x86_64"

ARM="-arch arm64"
x86="-arch x86_64"
ADDRESS_SANTIZER="-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer"

# Ensure output directory exists
mkdir -p ./bin

$CXX ./platform_backends/sdl_platform_layer.cpp \
  -std=c++11 \
  $DEBUG \
  $ARM \
  $WARNINGS \
  -I ./libs/GLAD/include \
  -iframework "$SDL_FRAMEWORK_SLICE" \
  -F "$SDL_FRAMEWORK_SLICE" \
  -framework SDL3 \
  -framework OpenGL \
  -Wl,-rpath,@executable_path/../Frameworks \
  -Wl,-rpath,"$SDL_FRAMEWORK_SLICE" \
  -o ./bin/Calculator
