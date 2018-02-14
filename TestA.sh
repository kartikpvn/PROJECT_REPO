# script for nodeA


echo "Server started"
echo " Start time"
date
sudo ./server /tmp/test_file.txt 10.1.1.2 10.1.1.3
echo "Server ended"
echo " End time"
date
echo " Client started"
echo " Start time"
date
sudo ./client /tmp/test_file_1.txt 10.1.1.2 10.1.1.3
echo "Client ended"
echo " End Time"
date
echo "md5sum for file sent and recieved" 
md5sum /tmp/test_file_1.txt /tmp/1g.txt
