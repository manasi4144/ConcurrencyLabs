# ConcurrencyLabs

Starter files and directions provided by Bernie Roehl of University of Waterloo:

 <h2>Lab 1</h2>

- cmd arg - capture command line input arguments
- images folder contains necessary sample images to test functions
- ls can be used to list all files under a directory and obtain file types
- png util provides a set of utility functions to process a PNG image file
- the pointer demonstrates how to use pointers to access a C structure
- the segfault contains a broken program that has a segmentation fault bug

<b>findpng</b> : search for all PNG files within specified directory (similar to find).
EXAMPLE:
findpng .
lab1/sandbox/new_bak.png
lab1/sandbox/t1.png
png_img/rgba_scanline.png
png_img/v1.png

OR
findpng: No PNG File Found

<b>catpng</b>: concatenate requested PNG images vertically to a new PNG named all.png
EXAMPLE:
The concatenated image should be named all.png.
catpng ./img1.png ./png/img2.png
Concatenate the listed PNG images vertically to all.png.

<b>pnginfo</b>: this function must print out the dimensions of a valid PNG image file. If the file does not exist or corrupted, an error message should be displayed.
EXAMPLE:
./pnginfo WEEF 1.png                             ./pnginfo red-green-16x16-corrupted.png
WEEF_1.png: 450 x 229                             red-green-16x16-corrupted.png: 16 x 16
                                                  IDAT chunk CRC error: computed 34324f1e, expected dc5f7b84
                                                  
 <h2>Lab 2</h2>

<b>paster</b>: Using libcurl and multiple threads, request a random horizontal strip of picture from server and when recieved all the strips, paste the downloaded png files together.
FLAGS:
-t=NUM : number of threads to create. If option not specified, single-threaded implementation of paster will run.
-n=NUM: Picture ID. Default value to 1

EXAMPLE:
paster -t 6 -n 2


 <h2>Lab 3</h2>
<b>paster2</b>: Uses producer threads make requests to the lab web server until they fetch all 50 distinct image segments. Each consumer thread reads image segments out of the buffer, one at a time, and then sleeps for X milliseconds specified by the user in the command line. Then the consumer will process the received data and paste the segments together.=
FLAGS:
./paster2 B P C X N
 B: buffer size
 P: Number of producers
 C: Number of Consumers
 X : Milliseconds consumer threads sleeps
 N : Picture ID. Default value to 1
 
 
 <h2>Lab 4</h2>:
<b>findpng2</b>:  a multi-threaded web crawler using blocking I/O to search for PNG file URLs on the web. Start search from the specified SEED URL, find all PNG file URLs on the web server and return  results to file- png urls.txt. Print out the execution time. 
 FLAGS:
-t=NUM : number of threads to create.
-m=NUM: number of unique PNG URLs to find.
-v=LOGFILE: log all the visited URLs by the crawler.


 <h2>Lab 5</h2>
<b>findpng3</b>: using non-blocking I/O, cURL and a single thread, find all PNG URls on web server.Start search from the specified SEED URL, find all PNG file URLs on the web server and return  results to file- png urls.txt. Print out the execution time. 
 FLAGS:
-t=NUM : number of threads to create.
-m=NUM: number of unique PNG URLs to find.
-v=LOGFILE: log all the visited URLs by the crawler.



 




