
import os

from src.shell.pin import INF_ALL, INF_ARITY, INF_TYPE, inf_code_to_str, get_previous_step


class ScatTest(object):
    
    def __init__(self, **kwargs):
        self.src = dict()
        self.bin = dict()
        self.log = dict()
        if "proto" in kwargs.keys():
            if "src" in kwargs["proto"].keys():
                self.src["proto"] = kwargs["proto"]["src"]
            if "bin" in kwargs["proto"].keys():
                self.bin["proto"] = kwargs["proto"]["bin"]
            if "log" in kwargs["proto"].keys():
                self.log["proto"] = kwargs["proto"]["log"]


    def __log_file(self, bin_path, inf_code):
        return "{2}/{0}_{1}.log".format(os.path.basename(bin_path), inf_code_to_str(inf_code), self.log["proto"])


    def unit_test(self, src_path, bin_path, infer):
        for inf in [INF_ARITY, INF_TYPE]:
            infer(
                    inf, 
                    bin_path, 
                    "", 
                    self.__log_file(bin_path, inf), 
                    self.__log_file(bin_path, get_previous_step(inf)) if get_previous_step(inf) >= 0 else None,
                    # TODO change this (debug purpose only)
                    True,
                )


    def arity(self, infer):
        # Compile all 
        # TODO
        # First we need to run the inference
        # Then we 
        self.unit_test("test/proto/src/void__void.c", "test/proto/bin/void__void", infer)

