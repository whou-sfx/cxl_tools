#!/usr/bin/bash
max_loop=0xffffffff
#max_loop=2

for ((i=0; i<$max_loop; i++)); do
	sleep 1

	./rescanpcie.sh
	./check_sfx_cxl_dev.sh
	if [ $? != 0 ]; then
		printf "check_sfx_cxl_dev.sh failed!!!\n"
		rc=-1
		break
	else
		rc=0
	fi

	./test_mb.sh $i
	if [ $? != 0 ]; then
		printf "test_mb.sh failed!!!\n"
		rc=-1
		break
	else
		rc=0
	fi
done

if [ $rc == 0 ]; then
	printf "\n####################### a1 cxl probe test pass, loop: %d ######################\n" $i
else
	printf "\n####################### a1 cxl probe test failed!!! loop: %d ######################\n" $i
fi

exit $rc
