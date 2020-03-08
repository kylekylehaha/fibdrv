reset
set ylabel 'time(ns)'
set xlabel 'size'
set title 'runtime'
set term png enhanced font 'Verdana,10'

set output 'runtime_client.png'

plot [:][:] \
'fib_fast_doub.txt' using 1:2 with linespoints linewidth 2 title 'fast doubling', \
'fib_seq.txt' using 1:2 with linespoints linewidth 2 title 'sequence'   
