#-*- coding: utf-8 -*-

import matplotlib.pyplot as plt  
import matplotlib.colors as colors
import pandas as pd  

class ArityChart(object):

    colors = [(31, 119, 180), (174, 199, 232), (255, 127, 14), (255, 187, 120),    
        (44, 160, 44), (152, 223, 138), (214, 39, 40), (255, 152, 150),    
        (148, 103, 189), (197, 176, 213), (140, 86, 75), (196, 156, 148),    
        (227, 119, 194), (247, 182, 210), (127, 127, 127), (199, 199, 199),    
        (188, 189, 34), (219, 219, 141), (23, 190, 207), (158, 218, 229)]

    def __init__(self, logfile):
        self.__log = logfile
        self.__data = dict()
        self.__parse_log()
        self.__data = sum(self.__data.values(), list())

    def __parse_log(self):
        with open(self.__log, "r") as f:
            for line in f.readlines():
                pgm = line.split(":")[0]
                self.__data.setdefault(pgm, list())
                entry = ArityEntry(line)
                self.__data[pgm].append(entry)

    def get(self, param):
        self.__data.sort(key=lambda a: a.get(param))
        data = dict()
        val = self.__data[0].get(param)
        tot = 0
        fp = 0
        fn = 0
        for e in self.__data:
            if e.get(param) != val and tot != 0:
                data[val] = ((tot - fp - fn) / float(tot), fp, fn, tot)
                val = e.get(param)
                tot = 0
                fp = 0
                fn = 0
            fn += e.fn_in
            fp += e.fp_in
            tot += e.tot_in
        return data

    def draw(self, data):
        plt.figure(figsize=(12, 9)) 
        ax = plt.subplot(111)    
        ax.spines["top"].set_visible(False)    
        ax.spines["bottom"].set_visible(False)
        ax.spines["right"].set_visible(False)    
        ax.spines["left"].set_visible(True)
        plt.plot(data.keys(), [1] * len(data.keys()), "-", lw=0.5, color="black")
        plt.plot(data.keys(), [0] * len(data.keys()), "-", lw=0.5, color="black")
        # Ensure that the axis ticks only show up on the bottom and left of the plot.    
        # Ticks on the right and top of the plot are generally unnecessary chartjunk.    
        ax.get_xaxis().tick_bottom()    
        ax.get_yaxis().tick_left()    

        # Limit the range of the plot to only where the data is.    
        # Avoid unnecessary whitespace.    
        plt.ylim(-0.1, 1.1)    
        plt.xlim(-1, max(data.keys()) + 1)

        plt.tick_params(axis="both", which="both", bottom="off", top="off",    
                labelbottom="on", left="off", right="off", labelleft="on") 
        acc = [v[0] for v in data.values()]
        fn = [v[1] for v in data.values()]
        norm = colors.Normalize(0, max(fn))
        fn = map(lambda a: norm(a), fn)
        fp = [v[2] for v in data.values()]
        norm = colors.Normalize(0, max(fp))
        fp = map(lambda a: norm(a), fp)
        tot = [v[3] for v in data.values()]
        norm = colors.Normalize(0, max(tot))
        tot = map(lambda a: norm(a), tot)
        plt.plot(data.keys(), acc, 'x', lw=1, color="blue", label="accuracy")
        plt.plot(data.keys(), fn, 'x', lw=1, color="red", label="false negatives")
        plt.plot(data.keys(), fp, 'x', lw=1, color="green", label="false positives")
        plt.plot(data.keys(), tot, 'x', lw=1, color="orange", label="number of functions")
        plt.legend()

        plt.show()
        plt.savefig("percent-bachelors-degrees-women-usa.png", bbox_inches="tight") 

class ArityEntry(object):

    def __init__(self, line, *args, **kwargs):
        l = line[:-1].split(":")
        self.__mincalls, self.__paramth, self.__retth = l[1:4]
        self.__fn_in, self.__fp_in, self.__tot_in = l[4:7]
        self.__fn_out, self.__fp_out, self.__tot_out = l[7:]
        super(ArityEntry, self).__init__(*args, **kwargs)

    @property
    def min_calls(self):
        return int(self.__mincalls)
        
    @property
    def param_threshold(self):
        return self.__paramth

    @property
    def ret_threshold(self):
        return self.__retth

    @property
    def fn_in(self):
        return int(self.__fn_in)
    
    @property
    def fp_in(self):
        return int(self.__fp_in)
        
    @property
    def tot_in(self):
        return int(self.__tot_in)

    def get(self, param):
        if param == "min_calls":
            return self.min_calls
        elif param == "ret_threshold":
            return self.ret_threshold
        elif param == "param_threshold":
            return self.param_threshold

