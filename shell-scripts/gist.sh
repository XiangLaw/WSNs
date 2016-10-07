while true; do 

cd ~/pastebin;
ls ~/trace-nn > out1.txt;
ls ~/trace-n1 > out2.txt;
gist -p out1.txt;
gist -p out2.txt;
sleep 1h
done