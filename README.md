# CXWindowManager



## Todo's (features and questions)
1. Setup specific key & key combos for window manager actions such as:
   - [x] Rotate window left/right
   - [x] Rotate layout in client tile-pair
   - [x] Move window left/right
   - [ ] Move window up/down
   - [x] Teleport window to (implementation done, user functionality not yet done)
   - [ ] Kill client & application
   - [ ] Custom configuration
   - [ ] Resize window
   - [ ] Implement EWMH stuff, to add client names as tags to our internal representation of windows
   - [ ] Font stuff
   - [ ] Basic graphics stuff, for example
        - [ ] Drawing a background
        - [ ] How does pixmaps work?
   - [ ] Logging of various X information (perhaps glance at basic_wm?) 

 
## Todo's implementation details

   - [x] Grab WM Hints and WM atoms etc. Can we get client names, so we can use them as identifiers? 