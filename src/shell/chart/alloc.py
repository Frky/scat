#-*- coding: utf-8 -*-

import os
import sys
import matplotlib.pyplot as plt  
import matplotlib.colors as colors
import pylab as P
import pandas as pd  

from .chart import Chart
from .entry.alloc import AllocEntry

class AllocChart(Chart):

    def __init__(self, oracle, *args, **kwargs):
        super(AllocChart, self).__init__(*args, **kwargs)
        with open("test/coreutils.txt", "r") as f:
            self.__coreutils = [line[:-1] for line in f.readlines()]
        self._analysis = "alloc"
        self.__parse_log()
        self._data = sum(self._data.values(), list())
        self.__oracle = oracle
 
    def __parse_log(self):
        if not os.path.exists(self._log):
            return
        with open(self._log, "r") as f:
            for line in f.readlines():
                pgm = line[:-1].split(":")[0]
                self._data.setdefault(pgm, list())
                entry = AllocEntry(line)
                self._data[pgm].append(entry)

    def __ok_or_ko(self, pgm, res, entry):
        if res == "None":
            return "n.c."
        try:
            if self.__oracle[pgm][entry] is not None and res in self.__oracle[pgm][entry]:
                return "\\checked"
            else:
                return "\\texttimes"
        except KeyError:
            return "n.c."

    def get(self, pgm=None):
        if pgm is None:
            return self._data
        else:
            return filter(lambda a: a.pgm == pgm, self._data)

    def table(self):
        tot = {
                    "alloc": {
                                "\\texttimes": 0,
                                "n.c.": 0,
                                "\\checked": 0,
                            },
                    "free": {
                                "\\texttimes": 0,
                                "n.c.": 0,
                                "\\checked": 0,
                            },
                }

        for e in sorted(self._data, key=lambda a:a.pgm):
            if e.pgm not in self.__coreutils:
                continue
            if e.alloc == "None":
                continue
            print "{{\\tt {}}} & {}/{} & {:.3g} & {:.3g} & {:.3g}/{:.3g} \\\\".format(
                        e.pgm, 
                        self.__ok_or_ko(e.pgm, e.alloc, "alloc"), 
                        self.__ok_or_ko(e.pgm, e.free, "free"), 
                        e.error_rate,
                        e.online, 
                        e.offline[0],
                        e.offline[1],
                    )
            tot["alloc"][self.__ok_or_ko(e.pgm, e.alloc, "alloc")] += 1
            tot["free"][self.__ok_or_ko(e.pgm, e.free, "free")] += 1
        for e in sorted(self._data, key=lambda a:a.pgm):
            if e.pgm in self.__coreutils:
                continue
            print "{{\\tt {}}} & {}/{} & {:.3g} & {:.3g} & {:.3g}/{:.3g} \\\\".format(
                        e.pgm, 
                        self.__ok_or_ko(e.pgm, e.alloc, "alloc"), 
                        self.__ok_or_ko(e.pgm, e.free, "free"), 
                        e.error_rate,
                        e.online, 
                        e.offline[0],
                        e.offline[1],
                    )
        print tot


    def table_cmp(self, other):
        for c in sorted(self._data, key=lambda a:a.pgm):
            t = other.get(c.pgm)[0]
            if c.pgm in self.__coreutils:
                continue
            print "{{\\tt {}}} & {}/{} & {}/{} & {:.3g} & {:.3g} & {:.3g}/{:.3g} & {:.3g}/{:.3g} \\\\".format(
                        c.pgm, 
                        self.__ok_or_ko(c.pgm, c.alloc, "alloc"), 
                        self.__ok_or_ko(c.pgm, c.free, "free"), 
                        self.__ok_or_ko(t.pgm, t.alloc, "alloc"), 
                        self.__ok_or_ko(t.pgm, t.free, "free"), 
                        c.online, 
                        t.online, 
                        c.offline[0],
                        c.offline[1],
                        t.offline[0],
                        t.offline[1],
                    )

    def draw_consistency(self):
        data = dict()
        for entry in self._data:
            data.setdefault(entry.pgm, list())
            data[entry.pgm].append(entry)

        plt.figure(figsize=(12, 9)) 
        plt.rc('text', usetex=True)
        plt.rc('font', family='serif')
        plt.ylabel("consistency rate")

        ax = plt.subplot(111)    
        ax.spines["top"].set_visible(False)    
        ax.spines["bottom"].set_visible(False)
        ax.spines["right"].set_visible(False)    
        ax.spines["left"].set_visible(True)
             
        ax.get_xaxis().tick_bottom()    
        ax.get_yaxis().tick_left()    
        ax.xaxis.set_ticklabels([])

        plt.tick_params(axis="both", which="both", bottom="off", top="off",    
                labelbottom="on", left="off", right="off", labelleft="on") 
        
        N = 0
        for rank, (pgm, entries) in enumerate(data.items()):
            consistency_rate = map(lambda a: a.consistency, entries)
            color = Chart.generic_colors[rank % len(Chart.generic_colors)]
            # Plot each line separately with its own color, using the Tableau 20    
            # color set in order.    
            for e in entries:
                if self.__ok_or_ko(e.pgm, e.alloc, "alloc") == "\\checked" and \
                        self.__ok_or_ko(e.pgm, e.free, "free") == "\\checked":
                    if e.consistency >= 0.95:
                        color = Chart.colors["acc"]
                    else:
                        color = Chart.colors["fn"]
                else:
                    if e.consistency > 0.95:
                        if self.__ok_or_ko(e.pgm, e.alloc, "alloc") == "\\checked":
                            color = Chart.generic_colors[-1]
                        else:
                            print e.pgm, e.alloc, e.free
                            color = Chart.colors["tot"]
                    else:
                        color = Chart.colors["acc"]
                plt.plot(N, e.consistency, 'o', color=color, mec=color) 
                N += 1
            # plt.plot(range(N, N + len(error_rate)), error_rate, 'o',
            #        lw=0, color=color, label=pgm, mec=color)
            # plt.text(N, -0.05 * (1 + ((1 + rank) % 2)), pgm, color=color, fontsize=18)
            N += 1
            if rank < len(data.keys()) - 1:
                plt.plot((N - 1, N - 1), (0, 1), '--', color="black", alpha=0.3)
        
        xmin, xmax = -1, N 
        ymin, ymax = -0.1, 1.1

        plt.ylim(ymin, ymax)
        plt.xlim(xmin, xmax)
        plt.plot([xmin, xmax], [0.95, 0.95], "-", lw=0.5, color="black", alpha=0.5) 
        plt.plot([xmin, xmax], [0, 0], "-", lw=1, color="black") 

        plt.savefig("test/chart/alloc_consistency.png", bbox_inches="tight") 
        
