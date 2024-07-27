# -*- codeing = utf-8 -*-
# @Time : 2024/7/27 13:01
# @Author : jszmwq
# @File : Count_number.py
# @Software: PyCharm

filename1 = 'E:/raster_join/PIP-master/point_id_result.txt'  # 假设文件名为numbers.txt
file1 = open(filename1, 'r')
numbers1 = file1.read().strip().split("\n")
frequency1 = {}
for number in numbers1:
    num = int(number)
    if num in frequency1:
        frequency1[num] += 1
    else:
        frequency1[num] = 1

filename2 = 'E:/raster_join/PIP-master/point_id_result - 副本.txt'  # 假设文件名为numbers.txt
file2 = open(filename2, 'r')
numbers2 = file2.read().strip().split("\n")
frequency2 = {}
for number in numbers2:
    num = int(number)
    if num in frequency2:
        frequency2[num] += 1
    else:
        frequency2[num] = 1

print(sorted(frequency1.values()))
print(sorted(frequency2.values()))