#-*- coding: utf-8 -*-

import sys
import matplotlib.pyplot as plt  
import matplotlib.colors as colors
import pylab as P
import pandas as pd  

class Chart(object):

    colors = {
                "acc": "#2c3e50",
                "fn": "#bdc3c7",
                "fp": "#7f8c8d",
                "tot": "#c0392b",
            }

    def __init__(self, logfile):
        self._log = logfile
        self._analysis = ""
        self._data = dict()

    def contains(self, pgm=None, vals=None):
        for e in self._data:
            if pgm is None or e.pgm == pgm:
                same = True
                if vals is None: 
                    return True
                for k, v in vals.items():
                    if e.get(k) != v:
                        same = False
                if same:
                    return True
        return False

    def get(self, param, defaults, inp=True, outp=True):
        self._data.sort(key=lambda a: a.get(param))
        data = dict()
        val = self._data[0].get(param)
        tot = 0
        fp = 0
        fn = 0
        for e in self._data:
            skip = False
            for k, v in defaults.items():
                if k != param and e.get(k) != v:
                    skip = True
                    break
            if skip:
                continue
            if e.get(param) != val and tot != 0:
                data[val] = ((tot - fp - fn) / float(tot), fp, fn, tot)
                val = e.get(param)
                tot = 0
                fp = 0
                fn = 0
            if inp:
                fn += e.fn_in
                fp += e.fp_in
                tot += e.tot_in
            if outp:
                fn += e.fn_out
                fp += e.fp_out
                tot += e.tot_out
        data[val] = ((tot - fp - fn) / float(tot), fp, fn, tot)
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
        acc = [v[0] for v in data.values()]
        fp = [v[1]/float(v[3]) for v in data.values()]
        fn = [v[2]/float(v[3]) for v in data.values()]
        tot = [v[3] for v in data.values()]
        norm = colors.Normalize(0, max(tot))
        tot = map(lambda a: norm(a), tot)
        plt.plot(data.keys(), acc, 'o', lw=1, color=Chart.colors["acc"], label="accuracy")
        if "min_calls" in name or "min_vals" in name:
            plt.plot(data.keys(), tot, 'o', lw=1, color=Chart.colors["tot"], label="number of functions (normalized)")
        plt.plot(data.keys(), fn, 'o', lw=1, color=Chart.colors["fn"], label="false negatives (% of total)")
        plt.plot(data.keys(), fp, 'o', lw=1, color=Chart.colors["fp"], label="false positives (% of total)")
        plt.legend()

        plt.savefig("test/chart/{}_{}.png".format(self._analysis, name), bbox_inches="tight") 

    def get_accuracy(self):
        with open("test/coreutils.txt", "r") as f:
            coreutils = [line[:-1] for line in f.readlines()]
        data = list(self._data)
        coreres = [d for d in data if d.pgm in coreutils]
        data = [d for d in data if d.pgm not in coreutils]
        data.append(reduce(lambda a, b: a.merge(b), coreres[1:], coreres[0]))
        data[-1].set_pgm("coreutils")
        cc = [d for d in data if "8cc" in d.pgm]
        data = [d for d in data if not "8cc" in d.pgm]
        data.append(reduce(lambda a, b: a.merge(b), cc[1:], cc[0]))
        data[-1].set_pgm("8cc")
        return data

    def draw_accuracy(self, data, name):
        tab = dict()
        tab["tot_in"] = dict()
        tab["fp_in"] = dict()
        tab["fn_in"] = dict()
        tab["acc_in"] = dict()
        tab["tot_out"] = dict()
        tab["fp_out"] = dict()
        tab["fn_out"] = dict()
        tab["acc_out"] = dict()
        order = ["acc_in", "acc_out", "fn_in", "fn_out", "fp_in", "fp_out", "tot_in", "tot_out"]
        for c, e in enumerate(data):
            for k in tab.keys():
                tab[k].setdefault(e.pgm, e.get(k))
        for k in order:
            sys.stdout.write("{} & ".format(k))
        sys.stdout.write("\\\\\n")
        for p in tab["tot_in"].keys():
            sys.stdout.write("{} & ".format(p))
            for k in order:
                if isinstance(tab[k][p], int):
                    sys.stdout.write("{} & ".format(tab[k][p]))
                else: 
                    sys.stdout.write("{0:.2f} & ".format(tab[k][p]))
            sys.stdout.write("\\\\\n")
            # if isinstance(total, int):
            #     sys.stdout.write("{} \\\\\n".format(total))
            # else:
            #     sys.stdout.write("{0:.2f} \\\\\n".format(total))
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
                sys.stdout.write("{} & ".format(total))
            else:
                sys.stdout.write("{0:.2f} &".format(total))
        sys.stdout.write("\\\\\n")
        return

        plt.figure(figsize=(12, 9)) 
        ax = plt.subplot(111)    
        ax.spines["top"].set_visible(False)    
        ax.spines["bottom"].set_visible(False)
        ax.spines["right"].set_visible(False)    
        ax.spines["left"].set_visible(False)
        # plt.plot([0, max(data.keys())*1.05], [1, 1], "-", lw=0.5, color="black")
        # plt.plot([0, max(data.keys())*1.05], [0, 0], "-", lw=0.5, color="black")
        # Ensure that the axis ticks only show up on the bottom and left of the plot.    
        # Ticks on the right and top of the plot are generally unnecessary chartjunk.    
        ax.get_xaxis().tick_bottom()    
        ax.get_yaxis().tick_left()    

        # Limit the range of the plot to only where the data is.    
        # Avoid unnecessary whitespace.    
        plt.ylim(-0.1, 1.1)    
        # plt.xlim(0, max(data.keys()) * 1.05)

        plt.tick_params(axis="both", which="both", bottom="off", top="off",    
                labelbottom="on", left="off", right="off", labelleft="on") 

        for c, e in enumerate(data):
            plt.plot([8*c], [e.acc_in], 'o', color=Chart.colors["acc"])
            plt.plot([8*c+2], [float(e.fn_in)/e.tot_in], 'o', color=Chart.colors["fn"])
            plt.plot([8*c+4], [float(e.fp_in)/e.tot_in], 'o', color=Chart.colors["fp"])

        plt.savefig("test/chart/{}_{}.png".format(self._analysis, name), bbox_inches="tight") 

    def get_var(self, pgm, defaults):
        data = list()
        for e in self._data:
            if pgm not in e.pgm:
                continue
            keep = True
            for k, v in defaults.items():
                if e.get(k) != v:
                    keep = False
                    break
            if keep:
                data.append(e)
        return data

    def draw_var(self, data, name):
        plt.figure(figsize=(12, 9)) 
        ax = plt.subplot(111)    
        bar_width = 0.5
        bar_l = [i + 1 for i in range(len(data))]
        tick_pos = [ i + (bar_width/2) for i in bar_l ]

        acc = map(lambda a: (a.acc_in*a.tot_in + a.acc_out*a.tot_out)/(a.tot_in + a.tot_out), data)
        fp = map(lambda a: float(a.fp_in + a.fp_out)/(a.tot_in + a.tot_out), data)
        fn = map(lambda a: float(a.fn_in + a.fn_out)/(a.tot_in + a.tot_out), data)

        ax.bar(bar_l, acc, width=bar_width, label="accuracy", 
                alpha=1, color=Chart.colors["acc"])
        ax.bar(bar_l, fn, width=bar_width, label="false negatives", 
                alpha=1, bottom=acc, color=Chart.colors["fn"])
        ax.bar(bar_l, fp, width=bar_width, label="false positives", 
                alpha=1, bottom=map(lambda a: a[0] + a[1], zip(acc, fn)), color=Chart.colors["fp"])
                
        # Limit the range of the plot to only where the data is.    
        # Avoid unnecessary whitespace.    
        plt.ylim(0.9, 1.01)    
        plt.xlim(0, len(data) * 1.05)

        ax.spines["top"].set_visible(False)    
        ax.spines["bottom"].set_visible(False)
        ax.spines["right"].set_visible(False)    
        ax.spines["left"].set_visible(False)

        # Ensure that the axis ticks only show up on the bottom and left of the plot.    
        # Ticks on the right and top of the plot are generally unnecessary chartjunk.    
        ax.get_xaxis().tick_bottom()    
        ax.get_yaxis().tick_left()    

        plt.xticks(tick_pos, map(lambda a: a.tot_in + a.tot_out, data), rotation="vertical")
        plt.tick_params(axis="both", which="both", bottom="off", top="off",    
                labelbottom="on", left="off", right="off", labelleft="on") 
        plt.legend()

        plt.savefig("test/chart/{}_{}.png".format(self._analysis, name), bbox_inches="tight") 

        return

        P.figure(figsize=(12,9))
        P.hist([[10, 20], [1, 5]], 5, stacked=True, histtype='bar')
        # plt.plot(range(1, len(data) + 1), acc_in, 'o', lw=1, color=Chart.colors[1], label="accuracy")
        # plt.plot(range(1, len(data) + 1), tot_in, 'o', lw=1, color=Chart.colors[3], label="number of functions")
        # plt.plot(range(1, len(data) + 1), fp_in, 'o', lw=1, color=Chart.colors[7], label="false positives")
        # plt.plot(range(1, len(data) + 1), fn_in, 'o', lw=1, color=Chart.colors[6], label="false negatives")
        # plt.legend()

        P.savefig("test/chart/{}_{}.png".format(self._analysis, name), bbox_inches="tight") 
        return
