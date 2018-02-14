# script for nodeB

echo " Start time"
date
echo "client started"
sudo ./client /tmp/test_file_1.txt 10.1.1.3 10.1.1.2
echo "client ended"
echo "End time"
date
sleep 5
echo " server started"
echo "Start Time"
date
sudo ./server /tmp/test_file.txt 10.1.1.2 10.1.1.3
echo "server ended"
echo " End Time"
date
echo "md5sum for file sent and recieved" 
md5sum /tmp/test_file.txt /tmp/1g.txt
rm /tmp/test_file.txt
