
def num_of_second(hour, minute):
    number_of_second = (minute * 60) + (hour * 360)
    return number_of_second


print(num_of_second(1, 10))

def uniq_list(my_list):
    final_list = list(dict.fromkeys(my_list))
    return final_list

print(uniq_list([1,1,1,2,3,4,5,6,7,7,8,]))