Comments (
	These are comments
	The way I plan to define my "dev docs" system, is so that I can put "todo's" here, describe and define them neatly, and also make them accessible directly from code.
	I don't want to have to go back and forth, between different systems and applications to look at what I need, want or should do, but instead be able to see 
	this in my code;
	TODO(Title, Subtitles...): Short description
	And then let a tool (I have not yet written) take that TODO, scan it's title and eventual subtitles, when I run a "devdoc" command, and take me directly to 
	the devdocs section where I have defined this title. This is an example devdoc file, and might be up for change how it will be supposed to be laid out. 
	For now, I am going to use things that look similar to markdown, but for efficiency there are probably a myriad of options that would be better.

	But since Markdown has easy Title and subtitle markdown code, I am going to use it for now.

	So let's say in my code I have the following in xcom/connect.cpp at line 97:
	// TODO(Workspace, Tiling): handle where, and what properties the to-be mapped window should have
	Running my theoretical tool/command over this line, it will scan that it has a TODO comment, read in the title and 
	a variadic list of subtitles. Perhaps these can be hashed for quick access to lines or whatever in the index
)

## Workspace
	Implement workspaces for my window manager. Looking at other similar applications, this is done in i3 for example, using
	some form of trees as a data structure. I am not sure I will follow that route, as it is very hacky and C-like. 
	Perhaps, I can use a more functional programming approach of a tree, using std::variant, where instead of a nodes-all-the-way-down approach,
	have each node, be a std::variant<container, node>, both of which types should be "mappable" in the XCB sense. Or maybe they should all be nodes,
	the general structure should however be something like below.

	So when for example, we have one window mapped, full screen the tree looks like:

						   [client_a]
							   *
							 /	 \
						   null	 null

	When a new client wants to be mapped, client_b, the tree data structure handles it by "pushing down" client_a, 
	being the only window that is focused, to the left branch, and "mapping" the new client to the right branch, thus getting this:

						[container]
					 _______*_______
					/				\
				   /				 \
			  [client_a]		>[client_b] <- newly mapped client. Also focused (the > means it's focused)
				  *					 *
				/   \			   /   \
			  null  null		null    null

	
	Now, when client_c get's a request of being mapped, the tree will look like this, after the mapping:

							[container]
					 ___________*_______
					/					\
				   /					 \
			  [client_a]			[container] 	
				  *						 ___*___
				/   \					/		\
			  null  null		[client_b]	  >[client_c]
									*				*
								  /   \			  /   \
								null  null		null    null

	However, this approach will require a lot of mutable, in place operations on the nodes. This doesn't strike me
	as particularly good. But will it get the job done? Most likely. The mutability might be a necessity here.
	Which means that a node, will have to be able to be swapped in and out, copy constructed and all kinds of horrible things.
	This makes it so the pointers to the left and right child of each node, can't be std::unique_ptr as they are not
	copyable (for obvious reasons). So how to approach this?


	One of the added benefits of an approach like this however, seems to be that we can "cut" branches out of the tree,
	and place them into another tree. This is because the cut branch, will have a root, and this root can acquire data
	from the join-node, about "how much space on the screen do you have, for me to dish out to my children when I join with you"
	for example.

### Tiling
Tiling should come with some features like a layout system, something like layout vert, layout horiz

