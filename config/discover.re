open Configurator.V1;

type os =
  | Android
  | IOS
  | Linux
  | Mac
  | Windows;

let header_detect_system = {|
  #if __APPLE__
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE
      #define PLATFORM_NAME "ios"
    #else
      #define PLATFORM_NAME "mac"
    #endif
  #elif __linux__
    #if __ANDROID__
      #define PLATFORM_NAME "android"
    #else
      #define PLATFORM_NAME "linux"
    #endif
  #elif WIN32
    #define PLATFORM_NAME "windows"
  #endif
|};

let get_os = t => {
  let header = {
    let file = Filename.temp_file("discover", "os.h");
    let fd = open_out(file);
    output_string(fd, header_detect_system);
    close_out(fd);
    file;
  };
  let platform =
    C_define.import(
      t,
      ~includes=[header],
      [("PLATFORM_NAME", String)],
    );
  switch (platform) {
  | [(_, String("android"))] => Android
  | [(_, String("ios"))] => IOS
  | [(_, String("linux"))] => Linux
  | [(_, String("mac"))] => Mac
  | [(_, String("windows"))] => Windows
  | _ => failwith("Unknown operating system")
  };
};

let root = Sys.getenv("cur__root");
let c_flags = [
  "-I",
  Sys.getenv("SDL2_INCLUDE_PATH"),
  "-I",
  Filename.concat(root, "include"),
  "-I",
  Filename.concat(root, "src"),
];

let c_flags = os =>
  switch (os) {
  | Linux => c_flags @ ["-fPIC"]
  | Windows => c_flags @ ["-mwindows"]
  | _ => c_flags
  };

let libPath = "-L" ++ Sys.getenv("SDL2_LIB_PATH");

let ccopt = s => ["-ccopt", s];
let cclib = s => ["-cclib", s];

let flags = os =>
  switch (os) {
  | Android =>
    []
    @ ccopt(libPath)
    @ cclib("-lEGL")
    @ cclib("-lGLESv1_CM")
    @ cclib("-lGLESv2")
    @ cclib("-lSDL2")
  | IOS =>
    []
    @ ccopt(libPath)
    @ cclib("-lSDL2")
    @ ccopt("-framework Foundation")
    @ ccopt("-framework OpenGLES")
    @ ccopt("-framework UIKit")
    @ ccopt("-framework IOKit")
    @ ccopt("-framework CoreVideo")
    @ ccopt("-framework CoreAudio")
    @ ccopt("-framework AudioToolbox")
    @ ccopt("-framework Metal")
    @ ccopt("-liconv")
  | Linux =>
    []
    @ ccopt(libPath)
    @ cclib("-lGL")
    @ cclib("-lGLU")
    @ cclib("-lSDL2")
    @ cclib("-lX11")
    @ cclib("-lXxf86vm")
    @ cclib("-lXrandr")
    @ cclib("-lXinerama")
    @ cclib("-lXcursor")
    @ cclib("-lpthread")
    @ cclib("-lXi")
  | Mac =>
    []
    @ ccopt(libPath)
    @ cclib("-lSDL2")
    @ ccopt("-framework AppKit")
    @ ccopt("-framework Foundation")
    @ ccopt("-framework OpenGL")
    @ ccopt("-framework Cocoa")
    @ ccopt("-framework IOKit")
    @ ccopt("-framework CoreVideo")
    @ ccopt("-framework CoreAudio")
    @ ccopt("-framework AudioToolbox")
    @ ccopt("-framework ForceFeedback")
    @ ccopt("-framework Metal")
    @ ccopt("-framework Carbon")
    @ ccopt("-liconv")
  | Windows =>
    []
    @ ccopt(libPath)
    @ cclib("-lSDL2")
    @ cclib("-lgdi32")
    @ cclib("-subsystem windows")
  };

let c_library_flags = os =>
  switch (os) {
  | Windows => []
  | _ => [libPath, "-lSDL2"]
  };

let cxx_flags = os =>
  switch (os) {
  | Android
  | Linux => c_flags(os) @ ["-std=c++11"]
  | IOS
  | Mac => c_flags(os) @ ["-x", "objective-c++"]
  | Windows =>
    c_flags(os) @ ["-fno-exceptions", "-fno-rtti", "-lstdc++", "-mwindows"]
  };

let () =
  main(~name="c_test", t => {
    let os = get_os(t);
    Flags.write_sexp(
      "c_library_flags.sexp",
      c_library_flags(os),
    );
    Flags.write_sexp("c_flags.sexp", c_flags(os));
    Flags.write_sexp("cxx_flags.sexp", cxx_flags(os));
    Flags.write_sexp("flags.sexp", flags(os));
  });
