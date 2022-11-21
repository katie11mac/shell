ls
wc -w < input.txt
rm output.txt
ls -l > output.txt
ls -a >> output.txt
ls | wc
wc < input.txt > output.txt
cat sortthis.txt | sort | uniq
cat sortthis.txt | sort | uniq | wc
ls | wc > output.txt
wc < input.txt | wc > output.txt
sort < input.txt | uniq | wc > output.txt