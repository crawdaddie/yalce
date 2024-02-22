# del_times_44100 = [1617, 1557, 1491, 1422, 1356, 1277, 1188, 1116]
#
# del_times_48000 = [x * 48000 / 44100 for x in del_times_44100]
#
# print([f"{x:.2f}" for x in del_times_48000])

sc = [0, 2, 3, 5, 7, 8, 10, 12]


def semi_to_ratio(semi):
    return 2 ** (semi / 12.0)


semi_to_ratio(2)


freqs = [220.0 * semi_to_ratio(x) for x in sc]

freqs
