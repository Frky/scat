#-*- coding: utf-8 -*-

import os
import sys
import matplotlib.pyplot as plt  
import matplotlib.colors as colors
import pylab as P
import pandas as pd  

from .chart import Chart
from .entry.couple import CoupleEntry

class CoupleChart(Chart):

    def __init__(self, *args, **kwargs):
        super(CoupleChart, self).__init__(*args, **kwargs)
        self._analysis = "couple"
        self.__parse_log()
        self._data = sum(self._data.values(), list())

    def __parse_log(self):
        if not os.path.exists(self._log):
            return
        with open(self._log, "r") as f:
            for line in f.readlines():
                pgm = line.split(":")[0]
                self._data.setdefault(pgm, list())
                entry = CoupleEntry(line)
                self._data[pgm].append(entry)

    def get(self, param, defaults):
        self._data.sort(key=lambda a: a.get(param))
        data = dict()
        val = -1
        tot = 0
        f, g, n = 0, 0, 0
        for e in self._data:
            skip = False
            for k, v in defaults.items():
                if k != param and e.get(k) != v:
                    print k, e.get(k), v
                    skip = True
                    break
            if skip:
                continue
            if e.get(param) != val:
                data[val] = (tot, f, g, n)
                val = e.get(param)
                tot = 0
                f = 0
                g = 0
                n = 0
            tot += e.tot
            f += e.f
            g += e.g
            n += e.n
        data[val] = (tot, f, g, n)
        return data

    def get_accuracy(self):
        with open("test/coreutils.txt", "r") as f:
            coreutils = [line[:-1] for line in f.readlines()]
        data = list(self._data)
        coreres = [d for d in data if d.pgm in coreutils]
        data = [d for d in data if d.pgm not in coreutils]
        nb_data = len(coreres)
        data.append(reduce(lambda a, b: a.merge(b), coreres[1:], coreres[0]))
        data[-1].set_pgm("coreutils")
        data[-1].average(nb_data)
        cc = [d for d in data if "8cc" in d.pgm]
        data = [d for d in data if not "8cc" in d.pgm]
        nb_data = len(cc)
        data.append(reduce(lambda a, b: a.merge(b), cc[1:], cc[0]))
        data[-1].set_pgm("8cc")
        data[-1].average(nb_data)
        return data

    def draw(self, data, name):
        plt.figure(figsize=(12, 9)) 
        ax = plt.subplot(111)    
        ax.spines["top"].set_visible(False)    
        ax.spines["bottom"].set_visible(False)
        ax.spines["right"].set_visible(False)    
        ax.spines["left"].set_visible(False)
        plt.plot([0, max(data.keys())*1.05], [1, 1], "-", lw=0.5, color="black")
        plt.plot([0, max(data.keys())*1.05], [0, 0], "-", lw=0.5, color="black")
        # Ensure that the axis ticks only show up on the bottom and left of the plot.    
        # Ticks on the right and top of the plot are generally unnecessary chartjunk.    
        ax.get_xaxis().tick_bottom()    
        ax.get_yaxis().tick_left()    

        # Limit the range of the plot to only where the data is.    
        # Avoid unnecessary whitespace.    
        plt.ylim(-0.1, 1.1)    
        plt.xlim(0, max(data.keys()) * 1.05)

        plt.tick_params(axis="both", which="both", bottom="off", top="off",    
                labelbottom="on", left="off", right="off", labelleft="on") 
        tot = [v[0] for v in data.values()]
        norm = colors.Normalize(0, max(tot))
        tot = map(lambda a: norm(a), tot)
        f = [v[1] for v in data.values()]
        norm = colors.Normalize(0, max(f))
        f = map(lambda a: norm(a), f)
        g = [v[2] for v in data.values()]
        norm = colors.Normalize(0, max(g))
        g = map(lambda a: norm(a), g)
        n = [v[3] for v in data.values()]
        norm = colors.Normalize(0, max(n))
        n = map(lambda a: norm(a), n)

        # plt.plot(data.keys(), tot, 'o', lw=1, color=Chart.colors["tot"], label="functions analyzed (normalized)")
        plt.plot(data.keys(), n, 'o', lw=1, color=Chart.colors["couples"], label="number of couples (normalized)")
        plt.plot(data.keys(), f, 'o', lw=1, color=Chart.colors["left"], label="number of left operand (normalized)")
        plt.plot(data.keys(), g, 'o', lw=1, color=Chart.colors["right"], label="right operand (normalized)")

        plt.legend()

        plt.savefig("test/chart/{}_{}.png".format(self._analysis, name), bbox_inches="tight") 

    def draw_accuracy(self, data, name):
        tab = dict()
        tab["tot"] = dict()
        tab["f"] = dict()
        tab["g"] = dict()
        tab["n"] = dict()
        order = ["tot", "n", "f", "g"]
        for c, e in enumerate(data):
            for k in tab.keys():
                tab[k].setdefault(e.pgm, e.get(k))
        for k in order:
            sys.stdout.write("& {} ".format(k))
        sys.stdout.write("\\\\\n")
        for p in tab["tot"].keys():
            sys.stdout.write("{} ".format(p))
            for k in order:
                if not isinstance(tab[k][p], float):
                    sys.stdout.write("& {} ".format(tab[k][p]))
                else: 
                    sys.stdout.write("& {0:.2f} ".format(tab[k][p]))
            sys.stdout.write("\\\\\n")
        sys.stdout.write("TOTAL ")
        for k in order:
            if k == "acc_in":
                total = map(lambda a: a[0] * a[1], zip(tab[k].values(), tab["tot_in"].values()))
                total = sum(total)/float(sum(tab["tot_in"].values()))
            elif k == "acc_out":
                total = map(lambda a: a[0] * a[1], zip(tab[k].values(), tab["tot_out"].values()))
                total = sum(total)/float(sum(tab["tot_out"].values()))
            else:
                total = sum(tab[k].values())
            if isinstance(total, int):
                sys.stdout.write("& {} ".format(total))
            else:
                sys.stdout.write("& {0:.2f}".format(total))
        sys.stdout.write("\\\\\n")
        return

    def draw_var(self, data, name):
        plt.figure(figsize=(12, 9)) 
        ax = plt.subplot(111)    
        bar_width = 0.5
        bar_l = [i + 1 for i in range(len(data))]
        tick_pos = [ i + (bar_width/2) for i in bar_l ]

        couples = map(lambda a: a.n, data)
        f = map(lambda a: a.f, data)
        g = map(lambda a: a.g, data)

        ax.bar(bar_l, couples, width=bar_width, label="number of couples", 
                alpha=1, color=Chart.colors["couples"])
        ax.bar(bar_l, f, width=bar_width, label="number of left operands", 
                alpha=1, bottom=couples, color=Chart.colors["left"])
        ax.bar(bar_l, g, width=bar_width, label="number of right operands", 
                alpha=1, bottom=map(lambda a: a[0] + a[1], zip(couples, f)), color=Chart.colors["right"])
                
        # Limit the range of the plot to only where the data is.    
        # Avoid unnecessary whitespace.    
        # plt.ylim(0.9, 1.01)    
        plt.xlim(0, len(data) * 1.05)

        ax.spines["top"].set_visible(False)    
        ax.spines["bottom"].set_visible(False)
        ax.spines["right"].set_visible(False)    
        ax.spines["left"].set_visible(False)

        # Ensure that the axis ticks only show up on the bottom and left of the plot.    
        # Ticks on the right and top of the plot are generally unnecessary chartjunk.    
        ax.get_xaxis().tick_bottom()    
        ax.get_yaxis().tick_left()    

        # plt.xticks(tick_pos, map(lambda a: a.tot_in + a.tot_out, data), rotation="vertical")
        plt.tick_params(axis="both", which="both", bottom="off", top="off",    
                labelbottom="off", left="off", right="off", labelleft="on") 
        plt.legend()

        plt.savefig("test/chart/{}_{}.png".format(self._analysis, name), bbox_inches="tight") 
