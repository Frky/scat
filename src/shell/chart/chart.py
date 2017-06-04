#-*- coding: utf-8 -*-

import sys
import matplotlib.pyplot as plt  
import matplotlib.colors as colors
import pandas as pd  

class Chart(object):

    colors = [
                (0.12156862745098039, 0.4666666666666667, 0.7058823529411765), 
                (0.6823529411764706, 0.7803921568627451, 0.9098039215686274), 
                (1.0, 0.4980392156862745, 0.054901960784313725), 
                (1.0, 0.7333333333333333, 0.47058823529411764), 
                (0.17254901960784313, 0.6274509803921569, 0.17254901960784313), 
                (0.596078431372549, 0.8745098039215686, 0.5411764705882353), 
                (0.8392156862745098, 0.15294117647058825, 0.1568627450980392), 
                (1.0, 0.596078431372549, 0.5882352941176471), 
                (0.5803921568627451, 0.403921568627451, 0.7411764705882353), 
                (0.7725490196078432, 0.6901960784313725, 0.8352941176470589), 
                (0.5490196078431373, 0.33725490196078434, 0.29411764705882354), 
                (0.7686274509803922, 0.611764705882353, 0.5803921568627451), 
                (0.8901960784313725, 0.4666666666666667, 0.7607843137254902), 
                (0.9686274509803922, 0.7137254901960784, 0.8235294117647058), 
                (0.4980392156862745, 0.4980392156862745, 0.4980392156862745), 
                (0.7803921568627451, 0.7803921568627451, 0.7803921568627451), 
                (0.7372549019607844, 0.7411764705882353, 0.13333333333333333), 
                (0.8588235294117647, 0.8588235294117647, 0.5529411764705883), 
                (0.09019607843137255, 0.7450980392156863, 0.8117647058823529), 
                (0.6196078431372549, 0.8549019607843137, 0.8980392156862745)
            ]

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

    def get(self, param, defaults):
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
            fn += e.fn_in
            fp += e.fp_in
            tot += e.tot_in
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
        plt.plot(data.keys(), acc, 'o', lw=1, color=Chart.colors[1], label="accuracy")
        plt.plot(data.keys(), tot, 'o', lw=1, color=Chart.colors[3], label="number of functions")
        plt.plot(data.keys(), fn, 'o', lw=1, color=Chart.colors[7], label="false negatives (% of total)")
        plt.plot(data.keys(), fp, 'o', lw=1, color=Chart.colors[6], label="false positives (% of total)")
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
        for c, e in enumerate(data):
            for k in tab.keys():
                tab[k].setdefault(e.pgm, e.get(k))
        for k in tab["tot_in"].keys():
            sys.stdout.write("{} & ".format(k))
        sys.stdout.write("TOTAL \\\\\n")
        for k in tab.keys():
            sys.stdout.write("{} & ".format(k))
            average = 0
            for p in tab[k]:
                if isinstance(tab[k][p], int):
                    sys.stdout.write("{} & ".format(tab[k][p]))
                else: 
                    sys.stdout.write("{0:.2f} & ".format(tab[k][p]))
            if k == "acc_in":
                total = map(lambda a: a[0] * a[1], zip(tab[k].values(), tab["tot_in"].values()))
                total = sum(total)/float(sum(tab["tot_in"].values()))
            elif k == "acc_out":
                total = map(lambda a: a[0] * a[1], zip(tab[k].values(), tab["tot_out"].values()))
                total = sum(total)/float(sum(tab["tot_out"].values()))
            else:
                total = sum(tab[k].values())
            if isinstance(total, int):
                sys.stdout.write("{} \\\\\n".format(total))
            else:
                sys.stdout.write("{0:.2f} \\\\\n".format(total))
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
            plt.plot([8*c], [e.acc_in], 'o', color=Chart.colors[1])
            plt.plot([8*c+2], [float(e.fn_in)/e.tot_in], 'o', color=Chart.colors[7])
            plt.plot([8*c+4], [float(e.fp_in)/e.tot_in], 'o', color=Chart.colors[6])

        plt.savefig("test/chart/{}_{}.png".format(self._analysis, name), bbox_inches="tight") 
