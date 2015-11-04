#-*- coding: utf-8 -*-

import yaml

class ConfigFileError(Exception):
    pass

def check_required_fields(req, cfg, config_path):
    for section in req.keys():
        if section not in cfg.keys():
            print "*** {0} field not found in {1} -- aborting".format(section, config_path)
            raise ConfigFileError
        field = req[section]
        if isinstance(field, dict):
            check_required_fields(field, cfg[section], config_path)

def parse_config(config_path):
    """
        Read a yaml file and extact configuration information

    """
    try:
        with open(config_path, 'r') as ymlfile:
            cfg = yaml.load(ymlfile)
    except IOError:
        print "*** File {0} not found -- aborting".format(config_path)
        raise ConfigFileError
    req_struct = {
                    "pin": {
                                "path": "",
                                "pintool-src": {
                                                "arity": "",
                                                "type": "", 
                                                "couple": "",
                                    },
                                "pintool-obj": {
                                                "arity": "",
                                                "type": "", 
                                                "couple": "",
                                    },
                            },
                    "log": {
                            "path": "",
                        }
                }
    check_required_fields(req_struct, cfg, config_path)
    return cfg

