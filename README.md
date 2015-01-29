# kicad2csvbom
a small c-programm, that extracts the fields for a bill of material out of a KiCad netlist file into a tab-separated csv file

# compile
make

# usage
The extracted data is printed to stdout. So we use the '>'-operator, to write it into a csv file. 

One line for each reference: 
- kicad2csvbom test.net > test.csv 

References with the same value, footprint and part are pulled together in one line:
- kicad2csvbom --compressed test.net > test.csv

# install
install with a symlink to the bin-folder in a similar way:
  sudo ln -s /home/your-path-here/kicad2csvbom/kicad2csvbom /usr/bin/kicad2csvbom

# History
- 2015-01-28 initial release
- 2015-01-29 added --compressed <br />
consistant naming of project (kicad2csvbom) <br />
fixed severe bug in parser