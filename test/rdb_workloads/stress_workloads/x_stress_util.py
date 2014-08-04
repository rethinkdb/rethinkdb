import math, random, datetime

def perform_ignore_interrupt(f):
    while True:
        try:
            return f()
        except IOError as ex:
            if ex.errno != errno.EINTR:
                raise

# end_date should be of the format YYYY-MM-DD - default is today
# interval should be of the format NUMBER (day, month, year) - default is 1 month
# prob is the probability (0...1] of returning a more recent month (by power law)
class TimeDistribution:
    def __init__(self, end_date, interval, prob=0.8):
        self.prob = prob
        if end_date is None:
            self.end_date = datetime.date.today()
        else:
            (year, month, day) = end_date.split("-")
            self.end_date = datetime.date(int(year), int(month), int(day))

        if interval is None:
            self.interval_type = "month"
            self.interval_length = 1
        else:
            (length, self.interval_type) = interval.split(" ")
            self.interval_length = int(length)

        if self.interval_type not in ["day", "month", "year"]:
            raise RuntimeError("unrecognized time interval: %s" % self.interval_type)

    def shitty_power_law(self):
        res = 1
        r = random.random()
        d_prob = self.prob
        while r > d_prob:
            res += 1
            d_prob += (1 - d_prob) * self.prob
        return res

    def get(self):
        delta = self.shitty_power_law() * self.interval_length

        if self.interval_type == "day":
            start_date = self.end_date - datetime.timedelta(days=delta)
            end_date = start_date + datetime.timedelta(days=self.interval_length)
        elif self.interval_type == "month":
            # Subtract the delta from the start date
            start_date = datetime.date(self.end_date.year, self.end_date.month, 1)
            while start_date.month < delta:
                delta -= 12
                start_date = datetime.date(start_date.year - 1, start_date.month, start_date.day)
            start_date = datetime.date(start_date.year, start_date.month - delta, start_date.day)

            # Add the interval to the end date
            end_date = start_date
            interval = self.interval_length
            while end_date.month + interval > 12:
                interval -= 12
                end_date = datetime.date(end_date.year + 1, end_date.month, end_date.day)
            end_date = datetime.date(end_date.year, end_date.month + interval, end_date.day)
        elif self.interval_type == "year":
            start_date = datetime.date(self.end_date.year - delta, 1, 1)
            end_date = datetime.date(start_date.year + self.interval_length, 1, 1)

        return (start_date, end_date)

class Pareto:
    def __init__(self, num_values, alpha=1.161):
        self.num_values = num_values
        self.exponent = alpha / (alpha - 1)

    def get(self):
        r = random.random()
        f = 1 - math.pow(1 - r, self.exponent)
        return min(self.num_values - 1, int(self.num_values * f))
