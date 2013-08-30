import math, random

# prob is the probability (0...1] of returning a lower number
def shitty_power_law(prob):
    res = 1
    r = random.random()
    d_prob = prob
    while r > d_prob:
        res += 1
        d_prob += (1 - d_prob) * prob
    return res

class Pareto:
    def __init__(self, num_values, alpha=1.161):
        self.num_values = num_values
        self.exponent = alpha / (alpha - 1)

    def get(self):
        r = random.random()
        f = 1 - math.pow(1 - r, self.exponent)
        return min(self.num_values - 1, int(self.num_values * f))

