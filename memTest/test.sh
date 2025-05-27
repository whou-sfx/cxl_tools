# Make sure debug is turned on

setpci -s 81:00.0 0x008.l=0x00000001

gcc -o memTest memTest.cc
./memTest    0x4000000000 DEADBEEF
./memTest    0x4000000004 01234567
# This should cause the line to be read and merged with the two WR above
./memTest -r 0x400000003C 00000000
# Check that dirty bytes were properly merged
./memTest -R 0x4000000004 01234567
./memTest    0x400000003C 76543210

# This should cause the line to be evicted
./memTest -r 0x4000000040 CAFEBABE

# This should cause the line to be simply evicted without writeback but without a read either
./memTest -w 0x4000000084 AABBCCDD

# This will cause the line to be evicted. Check previous line is properly restored
./memTest -R 0x4000000000 DEADBEEF
./memTest    0x4000000004 FFEEFFEE
# This will cause the line to be evicted. Check previous line is properly restored
./memTest -R 0x4000000084 AABBCCDD
# This will cause the line to be evicted without writeback. Check previous line content restored
./memTest -R 0x4000000004 FFEEFFEE
./memTest -R 0x4000000000 DEADBEEF

# This should cause the line to be simply evicted without writeback
./memTest -w 0x4000000080 00000000
./memTest -w 0x4000000084 11111111
./memTest -w 0x4000000088 22222222
./memTest -w 0x400000008C 33333333
./memTest -w 0x4000000090 44444444
./memTest -w 0x4000000094 55555555
./memTest -w 0x4000000098 66666666
./memTest -w 0x400000009C 77777777
./memTest -w 0x40000000A0 88888888
./memTest -w 0x40000000A4 99999999
./memTest -w 0x40000000A8 AAAAAAAA
./memTest -w 0x40000000AC BBBBBBBB
./memTest -w 0x40000000B0 CCCCCCCC
./memTest -w 0x40000000B4 DDDDDDDD
./memTest -w 0x40000000B8 EEEEEEEE
# This shoudl cause automatic write-through of the line
./memTest -w 0x40000000BC FFFFFFFF

# Total expected: 5 line reads, 4 line writes
exit 0
./memTest -R 0x4000000000 -L 4096

