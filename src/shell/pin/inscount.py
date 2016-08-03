
import os
import subprocess
import shutil
from datetime import datetime

class Pintool(object):

    nb_pintools = 0

    def __init__(self, *args, **kwargs):
        # Check for required parameters in kwargs
        req_param = ["name", "src_path", "obj_path", "pinconf"]
        for param in req_param:
            if param not in kwargs.keys():
                print "ERROR: parameter {0} expected".format(param)
                raise Exception
        self.__name = kwargs["name"]
        self.__src_path = kwargs["src_path"]
        self.__obj_path = kwargs["obj_path"]
        self.__pinconf = kwargs["pinconf"]

        # LOG function provided?
        if "log" in kwargs.keys():
            self.log = kwargs["log"]
        else:
            # If not, use the local one
            self.log = self.__log

        # Previous step required?
        if "prev_step" in kwargs.keys():
            self.__prev_step = kwargs["prev_step"]
        else:
            self.__prev_step = None

        # Set pintool code
        self.__code = Pintool.nb_pintools
        # Increment number of pintools created
        Pintool.nb_pintools += 1

    @property
    def code(self):
        return self.__code

    def __str__(self):
        return self.__name

    def __log(self, msg):
        print msg

    @property
    def prev_step(self):
        """
            Which pintool does it require to be executed before?
            return the name of the inference that preceeds

        """
        return self.__prev_step

    def __cmd(self, binary, args, logfile, infile=None):
        if infile is not None:
            infile_opt = "-i {0}".format(infile)
        else:
            infile_opt = ""
        if "cli-options" in self.__pinconf.keys():
            cli_options = self.__pinconf["cli-options"]
        else:
            cli_options = ""
        return "{0} {4} -t {1} -o {2} -fn \"{7}\" {3} -- {5} {6}".format(
                                                                            self.__pinconf["bin"],
                                                                            self.__obj_path, 
                                                                            logfile, 
                                                                            infile_opt, 
                                                                            cli_options,
                                                                            binary, 
                                                                            " ".join(args), 
                                                                            self.__pinconf["function-mode"]
                                                                        )

    def __gen_logfile(self, binary):
        """
            Generate a name for a new log file. For example, for arity inference
            on "grep", this will return grep_arity_{timestamp}.log.

            @param binary       name of the binary for which we require a log

            @ret                the generated name for the log file.

        """
        timestamp = datetime.now().strftime("%s")
        return "{0}_{1}_{2}.log".format(os.path.basename(binary), str(self), timestamp)

    def launch(self, binary, args, infile=None, verbose=True):
        """
            Launch specified inference on binary given in parameter

            @param binary   the binary file to analyse (must be a valid path to
                            an executable)

            @param args     arguments to give to the binary

            @param infile   path to the file where previous inference result is
                            stored (must be a valid path)

            @param verbose  if True, print intermediate steps

        """
        # if inf_code == INF_BASE:
        #     cmd = "{0} {1}".format(binary, " ".join(args))
        # else:
        logfile = self.__gen_logfile(binary)
        cmd = self.__cmd(binary, args, logfile, infile)
        self.log(cmd, verbose)
        start = datetime.now()
        subprocess.call(cmd, shell=True)
        duration = datetime.now() - start
        self.log("Inference results logged in {0}".format(logfile), verbose)
        self.log("Execution time: {0}.{1}s".format(duration.seconds, duration.microseconds), verbose)


    def compile(self, force, debug, trace):
        """
            Compile all pintools needed

        """
        #TODO os independant

        force_flag = ''
        if force:
            force_flag = '-B'

        src_path, src_name = os.path.split(self.__src_path)
        obj_path, obj_name = os.path.split(self.__obj_path)
        obj_path = os.path.abspath(obj_path)

        obj_build_path = "{}/build".format(obj_path)
        obj_build_name = src_name[:-4]
        if debug or trace:
            if debug:
                obj_build_name += '-debug'
            if trace:
                obj_build_name += '-trace'
        else:
            obj_build_name += '-release'

        if not os.path.exists(obj_build_path):
            os.makedirs(obj_build_path)

        obj_build_file = "{}/{}.so".format(obj_build_path, obj_build_name)
        if os.path.exists(obj_build_file):
            mtime_before = os.stat(obj_build_file).st_mtime
        else:
            mtime_before = 0

        if "compile-flags" in self.__pinconf.keys():
            compile_flags = self.__pinconf["compile-flags"]
        else:
            compile_flags = ""
        cmd = "make {} PIN_ROOT='{}' SCAT_COMPILE_FLAGS='{}' OBJDIR='{}/' '{}/{}.so'".format(
                force_flag,
                self.__pinconf["path"],
                compile_flags,
                obj_build_path,
                obj_build_path,
                obj_build_name)

        self.log("Compiling pintool: {0} ...".format(src_name[:-4]))
        with open("/dev/null", 'w') as fnull:
            try:
                subprocess.check_call(cmd, cwd=src_path, shell=True, stdout=fnull)
                mtime_now = os.stat("{}/{}.so".format(obj_build_path, obj_build_name)).st_mtime
                shutil.copyfile(
                        '{}/{}.so'.format(obj_build_path, obj_build_name),
                        '{}/{}'.format(obj_path, obj_name))
                if mtime_before == mtime_now:
                    self.log("   => Up to date !")
                else:
                    self.log("   => Done !")
            except subprocess.CalledProcessError as error:
                self.log("/!\ Compilation exited with non-zero status {} /!\\\n\n".format(error.returncode))
