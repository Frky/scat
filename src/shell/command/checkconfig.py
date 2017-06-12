#-*- coding: utf-8 -*-

from .i_command import ICommand
from src.shell.utils import checkpath

class CheckConfigCmd(ICommand):
    """
        usage: checkconfig

        Check the configuration file, and in particular:
            - the log directory (path, permissions)
            - pin (path, permissions)
    """

    def __init__(self, logdir, pinpath, *args, **kwargs):
        self.__logdir = logdir
        self.__pin = pinpath
        super(CheckConfigCmd, self).__init__(*args, **kwargs)
        return

    def run(self, *args, **kwargs):
        #TODO check also pintool paths => call pin.check_config
        try:
            checkpath(self.__logdir, isdir=True)
            checkpath(self.__pin, isexec=True)
        except ValueError as e:
            self.stderr(str(e))
            self.stderr("check configuration failed -- aborting")
            return
        self.stdout("Configuration seems ok")
