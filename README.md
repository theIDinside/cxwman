# CXWindowManager



## Todo's (features and questions)
1. Setup specific key & key combos for window manager actions such as:
   - [x] Rotate window left/right
   - [x] Rotate layout in client tile-pair
   - [x] Move window left/right
   - [x] Move window up/down
   - [x] Teleport window to (implementation done, user functionality not yet done)
   - [ ] Kill client & application
   - [ ] Custom configuration
   - [ ] Resize window
   - [ ] Implement EWMH stuff
        - [x] add client names as tags to our internal representation of windows
   - [ ] Font stuff
   - [ ] Basic graphics stuff, for example
        - [ ] Drawing a background
        - [ ] How does pixmaps work?
   - [ ] Logging of various X information (perhaps glance at basic_wm?) 

 
## Todo's implementation details

   - [x] Grab WM Hints and WM atoms etc. Can we get client names, so we can use them as identifiers?
   
   
### How to implement

##### Handling event functions that take parameters 
As it is right now, key press events are handled by EventHandler, defined in manager.hpp. This is done by keeping
a std::map of KeyConfiguration->Pointer-To-Member-Functions. These however, take no parameters. So how to solve this issue?
I don't want an enormous if-then-else wall.

I guess one way to solve it, would be that before an action is handled by EventHandler, parameters can be registered
*with* the EventHandler (from Manager), that then calls the pointer to member function (which is a ptm in Manager) that 
takes no parameters, which then accesses the registered parameters from EventHandler (EventHandler as it is, is a local 
struct of Manager.)

There are so many things that are totally hack-n-slash right now as far as the design goes, but it's just to get things up
and running, and frankly, I don't know any better as of right now.

There could be better solutions and designs here, but for now, this is what I got. 