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
        - Now this might be tricky to implement. I want to be able to resize a single window, and in doing so
        not just the split ratio between itself and it's sibling. This might mean, that if the width is resized,
        it might not even be the sibling client, whose width also get's resized. 
        Another problem that might arise, is that once a client is resized, moving it to another place on the screen
        introduces another issue; it now must teleport there, swap places with the client who is laid out there,
        while resizing the client with whom it is beside now, and also resizing it's prior sibling.
        
        One way to possibly solve it: When resizing a window for example 10 pixels along the x-axis to the left,
        is to find the left-most position on the client being resized, create a temporary geometry that is x pixels
        wider to the left, and then scan the workspace, looking for collisions. All clients that get collided with, get
        resized x amount of pixels.
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
As it is right now, key press events are handled by KeyEventHandler, defined in manager.hpp. This is done by keeping
a std::map of KeyConfiguration->Pointer-To-Member-Functions. These however, take no parameters. So how to solve this issue?
I don't want an enormous if-then-else wall.

I guess one way to solve it, would be that before an action is handled by KeyEventHandler, parameters can be registered
*with* the KeyEventHandler (from Manager), that then calls the pointer to member function (which is a ptm in Manager) that 
takes no parameters, which then accesses the registered parameters from KeyEventHandler (KeyEventHandler as it is, is a local 
struct of Manager.)

There are so many things that are totally hack-n-slash right now as far as the design goes, but it's just to get things up
and running, and frankly, I don't know any better as of right now.

There could be better solutions and designs here, but for now, this is what I got. 