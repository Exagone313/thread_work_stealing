#!/usr/bin/env python3
# vim: et ts=2 sw=2
import matplotlib.pyplot as plt
import numpy as np

threads = [1, 2, 4, 5, 8, 9, 16, 17]
data = {
  'linux-gcc8': {
    'serial': 0.331754,
    'lifo': [0.345265, 0.39695, 0.365165, 0.382522, 0.435989, 0.432104, 0.442492, 0.461857],
    'wsteal': [0.345232, 0.239279, 0.16831, 0.196361, 0.19076, 0.194968, 0.236196, 0.175339]
  },
  'linux-clang6': {
    'serial': 0.247794,
    'lifo': [0.260209, 0.381959, 0.387492, 0.425751, 0.460883, 0.44957, 0.473221, 0.47608],
    'wsteal': [0.26625, 0.182514, 0.163074, 0.11528, 0.156685, 0.206965, 0.224632, 0.196483]
  },
  'freebsd-clang6': {
    'serial': 0.249068,
    'lifo': [0.292721, 0.491081, 0.55717, 0.537308, 0.538336, 0.54773, 0.590261, 0.621939],
    'wsteal': [0.306579, 0.24216, 0.234856, 0.236745, 0.243924, 0.24395, 0.26757, 0.297211]
  },
  'freebsd-gcc6': {
    'serial': 0.328699,
    'lifo': [0.442596, 0.428111, 0.480874, 0.470629, 0.469542, 0.480541, 0.54101, 0.541344],
    'wsteal': [0.451131, 0.254518, 0.17239, 0.188442, 0.219321, 0.223537, 0.243589, 0.238495]
  },
  'freebsd-gcc8': {
    'serial': 0.337391,
    'lifo': [0.36534, 0.50306, 0.556256, 0.559473, 0.560871, 0.574971, 0.614861, 0.640707],
    'wsteal': [0.380726, 0.26398, 0.231822, 0.24181, 0.245622, 0.24964, 0.279063, 0.280447]
  },
}

plt.figure(figsize=(16.0, 12.0))
plt.xticks(threads)

for t in threads:
  plt.axvline(t, ls=':')

for platform in data.keys():
  plt.plot(threads, [data[platform]['serial'] for i in range(len(threads))], label=platform + '-serial')
  plt.plot(threads, data[platform]['lifo'], label=platform + '-lifo', ls='--', marker='o')
  plt.plot(threads, data[platform]['wsteal'], label=platform + '-wsteal', ls='--', marker='o')

plt.xlabel('threads')
plt.ylabel('time (s)')

plt.title("Time used for each strategy per number of threads")

plt.legend(loc='center right', bbox_to_anchor=(0.8, 0.5))

plt.savefig('stats.png')
