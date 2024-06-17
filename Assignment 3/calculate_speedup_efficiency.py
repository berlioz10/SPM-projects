import matplotlib.pyplot as plt


# a = ['seq', 1, 2, 4, 8, 16, 32, 40, 64, 128, 256, 512, 1024]

# list_0_1 = [5.644, 7.043209, 4.716364, 6.165470, 6.579945, 6.236188, 5.061379, 4.620733, 4.722333, 5.093932, 5.459488, 5.975734, 6.724168]
# list_0_2 = [5.644, 13.743106, 8.650371, 9.415636, 9.235248, 8.762935, 7.122957, 6.743067, 7.103055, 7.473801, 7.679988, 8.346903, 8.929747]

# list_1000_1 = [19.95, 21.574891, 12.174552, 10.274005, 8.991257, 8.223375, 7.453345, 7.173131, 7.538461, 7.886755, 8.455255, 9.037024, 9.714987]
# list_1000_2 = [19.95, 27.771086, 16.189318, 13.827926, 11.377339, 10.318680, 9.263648, 9.689624, 9.859518, 10.630626, 10.691858, 10.903466, 11.451752]

# list_10000_1 = [149.0, 149.89934, 76.968112, 42.764422, 25.217933, 16.964710, 13.973876, 13.493656, 12.958057, 13.451671, 13.419762, 14.000823, 15.062127]
# list_10000_2 = [149.0, 153.911703, 80.566260, 45.988558, 28.262903, 19.207095, 15.158998, 15.564770, 15.524692, 16.080760, 16.285275, 16.492632, 17.204734]

# legends = ['Merge maps', 'Merge pairs']

# a = ['seq', 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024]

# list_0_1 = [5.644, 5.969628, 5.376354, 10.993103, 20.735858, 42.568686, 70.748709, 14.811076, 14.532024, 15.361328, 15.830313, 15.979054]
# list_0_2 = [5.644, 5.526813, 3.884835, 5.379649, 5.089135, 5.152572, 5.473451, 5.354320, 5.528957, 5.154383, 5.570496, 5.373204]

# list_1000_1 = [19.95, 20.318626, 12.511355, 11.267722, 12.981164, 43.911856, 68.345232, 15.274565, 15.063918, 15.408948, 16.034537, 16.558313]
# list_1000_2 = [19.95, 19.847409, 12.210528, 13.405172, 12.337211, 12.811663, 12.473824, 12.701707, 13.272404, 12.895967, 12.673246, 13.017182]

# list_10000_1 = [149.0, 149.182287, 77.353332, 43.451747, 24.649167, 16.318465, 44.392709, 17.625168, 18.008881, 18.324296, 18.700952, 19.322745]
# list_10000_2 = [149.0, 148.17648, 87.220352, 78.373545, 74.505589, 72.831701, 72.673144, 72.217141, 72.328781, 72.149977, 72.422019, 72.821611]

# legends = ['Tasks over lines', 'Tasks over docs']

a = ['seq', 1, 2, 4, 8, 16, 19, 20, 32, 40, 64, 128, 256]

list_0_1 = [5.644, 21.944493, 12.467176, 10.223654, 16.700174, 13.937742, 13.325467, 15.675852, 17.062934, 45.380310, 54.612221, 102.756764, 150.977688]
list_0_2 = [5.644, 7.769888, 5.744390, 3.808225, 3.271665, 3.912813, 3.872929, 3.401069, 4.181116, 3.875158, 4.885352, 5.688194, 6.914607]

list_1000_1 = [19.95, 31.845402, 16.183441, 10.939607, 13.925483, 13.887952, 14.020114, 15.436567, 19.303109, 21.245811, 62.925474, 82.289116, 141.243369]
list_1000_2 = [19.95, 22.003240, 12.370323, 7.377547, 5.200390, 4.822248, 4.622082, 4.640438, 5.151662, 5.070147, 4.923375, 5.909106, 6.500576]

list_10000_1 = [149.0, 158.475593, 78.852623, 39.434988, 21.416288, 14.712100, 14.262425, 16.370123, 22.117912, 20.792282, 34.113616, 56.513169, 112.840392]
list_10000_2 = [149.0, 147.971767, 75.739762,39.501191, 21.445317, 12.213292, 10.915961, 10.700726, 14.064122, 12.346935, 11.677907, 11.709699, 11.729655]

legends = ['All2All', '2 Map Reduce']


fig, axs = plt.subplots(3,3)
list_1 = list_0_1
list_2 = list_0_2
extra_work = 0

axs[0][0].set(xticks=[i for i in range(len(a))], xticklabels=a)
axs[0][0].plot(a, list_1, '-ro')
axs[0][0].plot(a, list_2, '-bo')
axs[0][0].title.set_text(f'Time for {extra_work}')
axs[0][0].legend(legends)
# plt.show()

# plt.figure()
axs[0][1].set(xticks=[i for i in range(len(a))], xticklabels=a)
axs[0][1].plot(a, [list_1[1] / el for el in list_1], marker='.', color='cyan')
axs[0][1].plot(a, [list_2[1] / el for el in list_2], marker='.', color='orange')
axs[0][1].title.set_text(f'Speedup for {extra_work}')
axs[0][1].legend(legends)
# plt.show()

# plt.figure()
axs[0][2].set(xticks=[i for i in range(len(a))], xticklabels=a)
axs[0][2].plot(a, [1.0] + [list_1[1] / (el * thread) for el, thread in zip(list_1[1:], a[1:])], marker='v', color='green')
axs[0][2].plot(a, [1.0] + [list_2[1] / (el * thread) for el, thread in zip(list_2[1:], a[1:])], marker='v', color='purple')
axs[0][2].title.set_text(f'Efficiency for {extra_work}')
axs[0][2].legend(legends)

list_1 = list_1000_1
list_2 = list_1000_2
extra_work = 1000

axs[1][0].set(xticks=[i for i in range(len(a))], xticklabels=a)
axs[1][0].plot(a, list_1, '-ro')
axs[1][0].plot(a, list_2, '-bo')
axs[1][0].title.set_text(f'Time for {extra_work}')
axs[1][0].legend(legends)
# plt.show()

# plt.figure()
axs[1][1].set(xticks=[i for i in range(len(a))], xticklabels=a)
axs[1][1].plot(a, [list_1[1] / el for el in list_1], marker='.', color='cyan')
axs[1][1].plot(a, [list_2[1] / el for el in list_2], marker='.', color='orange')
axs[1][1].title.set_text(f'Speedup for {extra_work}')
axs[1][1].legend(legends)
# plt.show()

# plt.figure()
axs[1][2].set(xticks=[i for i in range(len(a))], xticklabels=a)
axs[1][2].plot(a, [1.0] + [list_1[1] / (el * thread) for el, thread in zip(list_1[1:], a[1:])], marker='v', color='green')
axs[1][2].plot(a, [1.0] + [list_2[1] / (el * thread) for el, thread in zip(list_2[1:], a[1:])], marker='v', color='purple')
axs[1][2].title.set_text(f'Efficiency for {extra_work}')
axs[1][2].legend(legends)

list_1 = list_10000_1
list_2 = list_10000_2
extra_work = 10000

axs[2][0].set(xticks=[i for i in range(len(a))], xticklabels=a)
axs[2][0].plot(a, list_1, '-ro')
axs[2][0].plot(a, list_2, '-bo')
axs[2][0].title.set_text(f'Time for {extra_work}')
axs[2][0].legend(legends)
# plt.show()

# plt.figure()
axs[2][1].set(xticks=[i for i in range(len(a))], xticklabels=a)
axs[2][1].plot(a, [list_1[1] / el for el in list_1], marker='.', color='cyan')
axs[2][1].plot(a, [list_2[1] / el for el in list_2], marker='.', color='orange')
axs[2][1].title.set_text(f'Speedup for {extra_work}')
axs[2][1].legend(legends)
# plt.show()

# plt.figure()
axs[2][2].set(xticks=[i for i in range(len(a))], xticklabels=a)
axs[2][2].plot(a, [1.0] + [list_1[1] / (el * thread) for el, thread in zip(list_1[1:], a[1:])], marker='v', color='green')
axs[2][2].plot(a, [1.0] + [list_2[1] / (el * thread) for el, thread in zip(list_2[1:], a[1:])], marker='v', color='purple')
axs[2][2].title.set_text(f'Efficiency for {extra_work}')
axs[2][2].legend(legends)

fig.set_figwidth(30)
fig.set_figheight(20)
plt.savefig("image.png")
plt.show()