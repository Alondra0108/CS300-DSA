# CS 300: System Analysis and Design

This repo is part of my Computer Science portfolio for CS 300: System Analysis and Design at Southern New Hampshire University. It contains two artifacts that show both the analysis side of working with data structures and the practical coding side:
	
 •	Project One Artifact: Runtime and Memory Analysis of Vector, Hash Table, and Binary Search Tree. This shows how I compared data structures, looked at their tradeoffs, and decided which one made the most sense for the advising program.
	
 •	Project Two Artifact: A full C++ program (ProjectTwo.cpp) that loads course data, validates it, and prints the Computer Science courses in alphanumeric order. It also lets you search a course and view its prerequisites.

Together, these two pieces show that I can both analyze algorithms and data structures on paper and implement them in working software.

What was the problem you were solving in the projects for this course?

The main problem was figuring out how to design an advising system that could store course information, let users search for a course, and list courses in order. Project One was all about comparing different data structures and deciding which would be most efficient. Project Two was taking that choice and actually coding the program so it could run and meet the requirements.

How did you approach the problem? Consider why data structures are important to understand.

I approached it by testing and analyzing multiple data structures. Vectors are simple, but searching through them can be slow. A Binary Search Tree is good for sorted data, but it’s more complex to maintain. The Hash Table ended up being the best fit because it gave quick lookups and still let me sort when needed. Understanding these tradeoffs is important because the wrong choice can make a program inefficient or harder to maintain.

How did you overcome any roadblocks you encountered while going through the activities or project?

The biggest roadblocks were around input validation and messy data. For example, I had to deal with duplicate courses, unknown prerequisites, and even circular dependencies. I solved this by building multiple passes into my code: one for parsing, one for validation, and one for cycle detection. Breaking it down into steps made it easier to debug and made sure the program stayed stable even with bad data.

How has your work on this project expanded your approach to designing software and developing programs?

Before, I mostly thought about “does the program run?” Now I think more about the design before writing code. These projects showed me how important it is to plan data flow and responsibilities: parse the data, clean it up, insert it, and then present it. This kind of structure makes programs easier to follow and extend later.

How has your work on this project evolved the way you write programs that are maintainable, readable, and adaptable?

I now focus on writing cleaner code that someone else could read and understand. I make sure to normalize inputs, comment tricky parts, and keep functions small and focused. I also learned that adaptability is key, when requirements change, like needing alphanumeric output, I can handle it without rewriting everything. Writing in a modular and defensive way has improved the quality of my programs and makes them much easier to maintain.
