import sys  #allows access to command line
import glob #allows searching for files
import os
import json #comes with python 3.4

is_wivi = False
try:
    import tkinter as tk 
    from tkinter import filedialog
except ImportError:
    is_wivi = True
def get_input_file_list(autoConvertToDB = False):
    if is_wivi:
        if len(sys.argv) > 1:
            input = json.load(open(sys.argv[1]))
            if 'files' in input:
                return input["files"]
            return input['data_files']
        else:
            return None
    else:
        if len(sys.argv) > 1:
            filenames = glob.glob(sys.argv[1] + '/*.db')
        else:
            filenames = []

        if len(filenames) == 0:
            root = tk.Tk()
            root.withdraw()
            root.focus_force()
            root.wm_attributes('-topmost', 1)

            options = {}
            #options['initialdir'] = '{0}'.format(os.path.expanduser('~'))
            options['filetypes'] = [("Data files", "*.dat;*.log;*.mdf;*.mf4;*.db"), ('all files', '.*')]
            options['title'] = 'Select list of input data files and click open.'
            options['defaultextension'] = '.db'
            #filenames = tkFileDialog.askopenfilenames(parent=self.parent, **options)
            filenames = list(filedialog.askopenfilenames(**options))
        return [{"path": item} for item in filenames]

def get_config_file():
    if is_wivi:
        if len(sys.argv) > 2:
            return sys.argv[2]
        elif len(sys.argv) > 1:
            input = json.load(open(sys.argv[1]))
            if 'config_files' in input and len(input['config_files']) > 1:
                return input['config_files'][0]
            return None
        else:
            return None
    else:
        root = tk.Tk()
        root.withdraw()
        root.focus_force()
        root.wm_attributes('-topmost', 1)
        fileName = filedialog.askopenfilename(filetypes = (("Lookup files", "*.sl;*.asl"), ("Signal Lookup files", "*.sl"),  ("Aliased Signal Lookup files", "*.asl"), ("All files", "*.*")), title ="Select script config file (*.asl) and click open.")
        return fileName

def get_config_data():
    fileName = get_config_file()
    if fileName is not None:
        return json.load(open(fileName))
    else:
        return None
def get_output_path():
    if is_wivi:
        if len(sys.argv) > 1:
            input = json.load(open(sys.argv[1]))
            if 'output_dir' in input:
                return input['output_dir']
        return os.getcwd()
    else:
        if len(sys.argv) > 1:
            return sys.argv[1]
        elif len(selected_file) > 0:
            return os.path.dirname(selected_file)
        else:
            os.getcwd()

def update_progress(percent, message):
    if is_wivi:
        with open("progress.ipa", "a") as progFile:
            progFile.write(percent)
    else:
        if len(message):
            sys.stderr.write(message + "\n")
        sys.stderr.write("Percent done: {}\n".format(percent))
        
def is_running_on_wivi_server():
    return is_wivi
