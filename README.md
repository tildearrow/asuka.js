# asuka

the Advanced Sound Unit.

# what?

a custom sound system using SDL and Emscripten for the web.

# note

THE CODE IS A MESS! the pressure to open its source made me put it here, but everything is a mess. therefore I would like to introduce a massive disclaimer:

the **libsndfile** directory contains a HEAVILY MODIFIED version of the original library, and therefore is NOT the original library.
the changes made are detailed here:

- disables Vorbis and FLAC support, in order to reduce final .wasm file.
- makes compilation easier by using prebuilt libraries, rather 
