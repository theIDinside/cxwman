# CXWindowManager



## Todo's (features and questions)
1. Setup specific key & key combos for window manager actions such as:
   - [ ] Custom configuration
   - [ ] Implement EWMH stuff
        - [x] add client names as tags to our internal representation of windows
   - [ ] Font stuff
   - [ ] Basic graphics stuff, for example
        - [ ] Drawing a background
        - [ ] How does pixmaps work?
   - [ ] Logging of various X information (perhaps glance at basic_wm?)
   - [ ] Implement some form of [docking container](#docking-container) window, that will contain clickable widgets, for instance "go to workspace x"
   - [x] [IPC](#ipc), handling (for instance) being sent commands by other applications or processes 
   
### User action features
   - [x] Rotate window left/right
   - [x] Rotate layout in client tile-pair
   - [x] Move window left/right
   - [x] Move window up/down
   - [x] Teleport window to (implementation done, user functionality not yet done)
   - [x] Kill client & application
   - [ ] Make window floating
   - [ ] Anchor tree to window (make it tiled)
   - [x] Resize tiled window
   - [ ] Resize floating window (easy.. Just tell X to resize, since we don't care how or where it ends up)
   - [ ] Make a run command user input window (described under section ["User command input window"](#user-command-input-window))
   - [x] Change workspace
   - [ ] Tag workspaces so switching between them can be done via key-combos
   - [ ] Open run CXWMAN command (as an input box or something). For instance; kill x, or open x on ws1 things of that nature
 
### User command input window
    This will be implemented as a GTK (or any other toolkit) application.
#### Feature ideas
   - Input box, which can translate user input to commands currently bound to global key-combos
   - Execute shell commands 
    

### Docking container
Similar to that of windows or linux. Containing things like what workspace one is on, or showing sys applets etc. 
Simple stuff to begin with.

### IPC
Implemented using Unix Domain Sockets. Currently, we link to a library cxprotocol that is supposed to keep all the info
of how to send messages, create messages from payloads and decode them etc. Found in dep/local/cxprotocol.

Using this, we can start creating Window Manager client applications, that take advantage of toolkits, and thus
not have to "hardcode"/"handcode" in widget-y like things as we have done with the current workspace switcher. 

For example, we can create something that looks nice, behaves nice and just send IPC commands over the unix domain socket. 

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


#### Screenshot of current basic functionalities
![Tiled windows and extremely rudimentary status bar](misc/basic_functionality_and_look.png)



### Implementation details / Log
Added epoll to the WM setup. EPoll is used for I/O multiplexing and is a linux feature (POSIX only has select/poll)
By adding the XCB/X11 connection to the EPoll set (by calling xcb_get_filedescriptor(), we can multiplex on the
IPC Unix Domain Socket and the X11 socket, thus we can poll for events on the IPC sockets & the X server sockets simulataneously
thus increasing performance, since epoll_wait will actually block, and not be running an infinte loop absolutely chewing through CPU usage

Trying to implement some Command pattern feature, so that the IPC mechanism will simulate what the Manager class does,
and possibly get away from the enormouse if-then-else wall that one gets with handling input and making commands. Haven't really landed on anything good yet.

