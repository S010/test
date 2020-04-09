#!/usr/bin/python3
#
# https://en.wikipedia.org/wiki/Moving_average#Exponential_moving_average
#

numbers = [84.0, 22.0, 33.0, 59.0, 1.0, 38.0, 95.0, 67.0, 43.0]

def calc_ema(y, s, a, t):
    if t == 1:
        return y
    else:
        return a*y + (1-a)*s

def calc_avg(l):
    return float(sum(l))/float(len(l))

ema = 0.0
a = 0.5

for t in range(len(numbers)):
    ema = calc_ema(numbers[t], ema, a, t+1)
    avg = calc_avg(numbers[:t+1])
    print('avg=%.1f ema=%.1f numbers=%s' % (avg, ema, str(numbers[:t+1])))
