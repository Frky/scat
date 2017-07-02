#-*- coding: utf-8 -*-

class CoupleRes(object):

    def __init__(self):
        self.__f = 0
        self.__g = 0
        self.__nb = 0

    def add(self, res, pgm="", verbose=True):
        return
        if verbose:
            try:
                print "{}: {}/{} {:.2f}% ({} fn, {} fp) - {}/{} {:.2f}% ({} fn, {} fp)".format(
                        pgm, 
                        res[0],
                        res[-2],
                        res[0]*100./res[-2],
                        res[3],
                        res[2],
                        res[1],
                        res[-1],
                        res[1]*100./res[-1],
                        res[5], 
                        res[4], 
                            )
            except Exception:
                print "{}: n.c.".format(pgm)

    def __str__(self):
        return "{}: {}/{} {:.2f}% ({} fn, {} fp) - {}/{} {:.2f}% ({} fn, {} fp)".format(
                    "TOTAL", 
                    self.__in["ok"], 
                    self.__in["tot"], 
                    self.__in["ok"]*100./self.__in["tot"] if self.__in["tot"] else 0.0,
                    self.__in["falseneg"],
                    self.__in["falsepos"],
                    self.__out["ok"], 
                    self.__out["tot"], 
                    self.__out["ok"]*100./self.__out["tot"] if self.__out["tot"] else 0.0,
                    self.__out["falseneg"],
                    self.__out["falsepos"], 
                )

