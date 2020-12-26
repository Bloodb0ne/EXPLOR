# EXPLOR 

This is an implementation of the language EXPLOR, developed by Ken Knowlton. The only information that I found about it was in a paper in the UAIDE: Proceedings of the 9th Annual Meeting and decided to try and create a version in C++ 17 for fun and learning new features.

The project features a parser for the language, but its made with another experiment of mine so its not the most stable parsing solution, but its not tightly coupled.

The implementation of the language executution logic resides in `ExploreLang.h` and `ExploreTypes.h` where the main class
EXPLOR has a function `addLine()` to insert objects of type `Commands` with an optional label ( similar to a goto label ) that represent a pattern of an actual command.
Each Command has a probability, type and an optional goto label;

A more detailed explanation can be found [here](https://bloodb0ne.github.io/explore_language_revival.html)

# Usage
Single parameter thats the path to the source file we want to parse

`Explor.exe <path_to_source>`

On a successful syntesis of an image shows the output path, same file name as the source with a .pbm extension. PBM files are simple 1BPP B/W images.
In the folder `./examples` there are a couple of examples taken from the original paper.

# Notes
The current parsing is sensitive to some whitespace, does not support comments in the code and at the moment doesnt have nice error messages to report where ( contextually ) it occured. It does however show the line number and index where it occured.

# Requirements
C++17

