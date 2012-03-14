#!/usr/bin/python
import re
import os
import sys

frontend = "dialog"

result = os.popen('which ' + frontend)
if result.read() == '':
	print("\nDas gewuenschte Frontend (" + frontend + ") steht nicht zur Verfuegung.")
	print("Sie muessen ein anderes waehlen (editieren sie config.py), oder das gewaehlte installieren.\n")
	sys.exit(1)


class shift_register:
    def __init__(self, size):
        self.size = size
        self.data = {}
        
        for index in range(1, size):
            self.data[index] = ""
        
    def shift(self, value):
        for index in range(0, self.size - 1):
            self.data[self.size - index] = self.data[self.size - index - 1]
        
        self.data[1] = value
    
    def at(self, index):
        if index in range(1, self.size):
            return self.data[index]
        else:
            return ""

def show_dialog(options, define_name, define_value):
    title = "tyndur Konfigurieren"
    print(define_value)
    if options['type'] == 'yesno':

        if define_value == "undef":
            default = " --defaultno"
        else:
            default = ""

        status = os.system(frontend + default + ' --title "' + title + '"  --yesno "' + options['desc'] + '" 0 0')
        if status == 0:
            return "#define " + define_name
        else:
            return "#undef " + define_name
    elif options['type'] == 'text':
        result = os.popen(frontend + ' --stdout --title "' + title + '"  --inputbox "' + options['desc'] + '" 0 0 "' + define_value + '"').readline()
        
        return '#define ' + define_name + ' ' + result
    
    elif options['type'] == 'radio':
        opts = options['values'].split(',')
        
        opts_list = ''
        for opt in opts:
            opts_list = opts_list + ' "' + opt + '" "" '
            if opt == define_value:
                opts_list += 'on'
            else:
                opts_list += 'off'
            
        result = os.popen(frontend + ' --stdout --title "' + title + '"  --radiolist "' + options['desc'] + '" 0 0 ' + str(len(opts)) + '  ' + opts_list ).readline()
        return '#define ' + define_name + ' ' + result




line_buffer             = shift_register(10)
config_file_source      = open('src/include/lost/config.h','r')
lines                   = config_file_source.readlines()
config_file_dest        = open('src/include/lost/config.h','w')

regex_define            = re.compile('^\#(define|undef) +CONFIG')
regex_define_simple     = re.compile('^\#(define|undef) +(CONFIG_.*)')
regex_define_complex    = re.compile('^\#define +(CONFIG_.*?) (.*)')
regex_option            = re.compile('^\//%(.*?) ["\'`](.*?)["\'`]')


for _line in lines:
    line = _line.strip()
    line_buffer.shift(line)
    
    
    if(regex_define.match(line)):
        options = {}
        for index in range(2, line_buffer.size):
            if regex_define.match(line_buffer.at(index)):
                break
            
            option_match = regex_option.match(line_buffer.at(index))
            if option_match:
                options[option_match.group(1)] = option_match.group(2)
                
        
        match_define_complex = regex_define_complex.match(line)
        if(match_define_complex):
            define_name = match_define_complex.group(1)
            define_value = match_define_complex.group(2)
            print('define name:`' + define_name + '` value:`' + match_define_complex.group(2) + '`')
            _line = _line.replace(line, show_dialog(options, define_name, define_value))
            
        else:
            match_define_simple = regex_define_simple.match(line)

            is_defined          = match_define_simple.group(1)
            define_name         = match_define_simple.group(2)
            
            print('define name:`' + define_name + '` value:`' + is_defined + '`')
            
            _line = _line.replace(line, show_dialog(options, define_name, is_defined))
    
    config_file_dest.write(_line)



config_file_dest.close()
config_file_source.close()

print("\n\nAls naechster Schritt ist in den meisten Faellen ein 'make clean' notwendig.")
print("Anschliessend koennen mit 'make' der tyndur-Kernel und die Module erstellt werden.\n\n")
