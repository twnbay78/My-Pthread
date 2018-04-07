<h1>OS Land Raider </h1>
by Leo Scarano and Vincent Taylor

<h3>Segmented Paging FTW!</h3>

It is very useful in many cases to sort large sets of data. One good example of 
this is sorting a large set of movie data. Movies have several attributes involved, 
allowing for lots of data be analyzed, mined, sorted and distributed throughout wide 
arrays of applications.  This program, in short, will take in movie data from a .csv
file (most typically associated with Microsoft Office Excel) and sort the file based 
on a certain type of data.

<h3>Usage</h3>

Compiling and linking necessary files is made simple using the "make" command in 
terminal:
<br>
<br>
<pre>$make</pre>
<br>

Running the project is accomplished by outputting the data into standard input:
<br>
<br>
<pre>./some_executable</em></pre>
<br>

<h3>Phases</h3>

The following implementation has been created in 4 phases:

<ul>
	<li>Phase A: Direct-Mapped Memory</li> 
		o This part of the project deals with mapping memory request using malloc to a global char array of 8Mb. This effectively acts as our own memory, so we first must align the memory and decide how we are going to store the metadata and handle multiple concurrent request. 
	<li>Phase B: Virtual Memory</li>
		o In this stage we must split up the data into pages this way we can mprotect the data. We must decide on a page layout for our memory and how we are going to organize what thread owns what page. Afterwards, we use the malloc in part one to allocate within each page. Additionally, after protecting our memory we must create a signal handler that allows us to load the current thread’s pages in when there is a call to its memory i.e. a Page fault. 
	<li>Phase C: Swap File</li>
		o This stage requires us to introduce a swap file of 16mb which we can use as secondary storage to store more pages. This requires us to create a file and have an eviction policy. Furthermore, we must include a way to read, and write to the swap file. After all of this we must be able to find free pages in the swap file and move them into memory giving threads more memory if needed.
	<li>Phase D: Shared Region</li>
		o Finally, this part of the projects tasks us with creating a shared region of memory for all the threads used by the shalloc command. This region acts the same as all the other memory only it is not protected or owned by a single thread.
</ul>
<h3>Structure</h3>
Block / metadata
	Size - Size of allocated request
	isFree - Determines whether the metadata is free 
	nextPtr – Pointer to the next block if the data spans multiple blocks
	TID – TID of the data owner (Primarily Used in the Swap File)
Page
	TID - TID of the thread that owns the page
	isFree – Determines whether the page is free or not
	page - Pointer to the actual page  
	page_num - Number of the page (identifier)
	next - Pointer to the next page
Page Table membook
	The page table data structure for going though pages.
Memory Layout
[_Page Table 15 pages | temp region 1 page| Thread Page Counter 1 page |Shared Region 111 page| memory 1920 pages]
In depth look/description
	We purposely segment our memory into two chunks of 960 pages this allows us to swap more easily with the swap file which will also be in chunks of 960 pages. There is another rule we added to this, it is that no thread may possess more than 480 pages the reason for this will be explained later in our swap algorithm. 
Swapfile Layout
[_swap book 256 pages_| Swapfile contents 3840 pages] The large gap of swap book pages is due to the segmentation implementation we wanted to implement. Since space is bountiful we decided this was better than a longer runtime for swapping
Eviction Algorithm
Swapping Algorithm Structure
All our memory obeys these rules
1)	If one of a thread’s pages is in memory, then all of its pages are.
2)	If one of a thread’s pages is in the Swapfile then all of its pages are.
3)	At the time of swap all a threads pages will be in either the left chunk or the right chunk. (No thread can have pages in each segment of the memory)
4)	No Thread may own more than (1/4 of the current pages in memory in our case that is 480)
5)	The Swap file can only move segmented chunks between memory and itself
Sample Memory
{[69|69|12|2|12|] [1|4|4|5|1]}
Sample Swap File
{[9|6|6|7|9] [10|0|3|3|3] [11|11|11|11|11] [16|0|14|14|16]}
When we want to shift out a segment of our memory, so we must obey rule 3
	We swap the right chunk of memory with the 2nd chunk of the Swapfile
{[69|69|12|2|12|] [10|0|3|3|3]}
{[9|6|6|7|9] [1|4|4|5|1] [11|11|11|11|11] [16|0|14|14|16]}
Now we can call align pages to place the current threads pages into the front (we will use 3 as the TID, 0 is a free block)
memAllign
{[3|3|3|2|12|] [10|0|69|69|12]}
	Now if we want to swap we call our re-organize function and make which effectively memAllign the blocks to the left half of the memory
1)	 reorganize (3)
{[3|3|3|2|12|] [10|0|69|69|12]}
2)   reorganize (2)
{[3|3|3|2|12|] [10|0|69|69|12]}
3)   reorganize (12)
{[3|3|3|2|12|] [12|0|69|69|10]}
	However, we are in between two chunks, so we fail to copy and revert back
{[3|3|3|2|12|] [10|0|69|69|12]}
	4)   reorganize (10)
{[3|3|3|2|10|] [12|0|69|69|12]}
	Now since we have reached the end of the left chunk we can safely swap either or both chunks without worry.	
</div>
<br>
<br>
<br>
<br>
<br>
