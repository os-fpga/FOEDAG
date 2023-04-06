import sys
import os
import json

def check_case(name, upper) :

    assert isinstance(upper, bool)
    assert len(name) >= 2, "Name must be at least two characters, but found %s" % name
    for i, c in enumerate(name) :
        assert ((upper and c >= 'A' and c <= 'Z') or \
                (not upper and c >= 'a' and c <= 'z') or \
                (i > 0 and ((c >= '0' and c <= '9') or c == '_'))), \
                "Expect naming (%c) to be alphanumeric in %s case and character '_', first character must be alphabet, but found %s" % (c, "upper" if upper else "lower", name) 

class OPTION :

    def __init__(self, args, long_names, short_names) :

        # filling up
        assert "name" in args
        if "short" not in args :
            args["short"] = None
        assert "type" in args
        if "optional" not in args :
            args["optional"] = False
        if "multi" not in args :
            args["multi"] = False
        if "default" not in args :
            args["default"] = None
        assert "help" in args
        if "hide" not in args :
            args["hide"] = False
        # grep
        n = args["name"]
        s = args["short"]
        t = args["type"]
        o = args["optional"]
        m = args["multi"]
        d = args["default"]
        h = args["help"]
        hi = args["hide"]
        assert len(args) == 8
        # Manipulate
        e = None
        if t.find("|") != -1 :
            e = t.strip()
            t = "enum"
        # Checking
        assert isinstance(n, str)
        check_case(n, False)
        assert n != "help"
        assert n.find("m_") != 0, "Option name cannot start with string \"m_\""
        assert n not in long_names
        long_names.append(n)
        assert s == None or (isinstance(s, str) and \
                            len(s) == 1 and \
                            s[0] >= 'a' and \
                            s[0] <= 'z')
        self.short = s
        if s != None :
            assert s not in short_names
            assert s != "h"
            short_names.append(s)
            s = "'%s'" % s
        else :
            s = "char(0)"
        assert t in ["flag", "int", "str", "enum"]
        assert isinstance(o, bool)
        assert isinstance(m, bool) 
        if t == "enum" :
            assert isinstance(e, str)
            e = e.split("|")
            assert len(e) > 1
            for i in range(len(e)) :
                e[i] = e[i].strip()
                assert len(e[i])
            assert m == False
        else:
            if t == "flag" :
                assert m == False
            assert e == None
        if d == None :
            if t == "flag" :
                d = False
            elif t == "int" :
                if m :
                    d = None
                else :
                    d = 0
            elif t == "str" :
                if m :
                    d = None
                else :
                    d = ""
            else :
                d = e[0]
        else :
            if t == "flag" :
                assert isinstance(d, bool)
            elif t == "int" :
                assert isinstance(d, int)
            elif t == "str" :
                assert isinstance(d, str)
            else :
                assert d in e
        if isinstance(h, list) :
            assert len(h)
            for hl in h :
                assert len(hl)
                assert isinstance(hl, str)
        else :
            assert len(h)
            assert isinstance(h, str)
        assert isinstance(hi, bool)
        # Assign
        self.name = n
        self.short_name = s
        self.type = t
        self.optional = o
        self.multiple = m
        self.default = d
        self.help = h
        self.enum = e
        self.hide = hi

    def __str__(self) :

        if self.optional :
            if self.short == None :
                str = "{ %s}" % (self.name)
            else :
                str = "{ %s:%c }" % (self.name, self.short)
        else :
            if self.short == None :
                str = "< %s >" % (self.name)
            else :
                str = "< %s:%c >" % (self.name, self.short)
        if self.multiple :
            str = "%ss" % str
        return str

    def get_enum(self) :

        if self.enum == None :
            return "{}"
        else :
            string = "{ "
            for e in self.enum :
                string = "%s\"%s\", " % (string, e)
            string = "%s }" % string[:-2]
            return string

class database :

    def __init__(self, n, h, a) :

        assert isinstance(n, str)
        check_case(n, False)
        assert isinstance(h, str) or isinstance(h, list)
        if isinstance(h, list) :
            for s in h :
                isinstance(s, str)
        assert len(h)
        assert isinstance(a, list) and \
                len(a) == 2 and \
                isinstance(a[0], int) and \
                isinstance(a[1], int) and \
                a[0] >= 0 and \
                (a[1] >= a[0] or a[1] == -1)
        self.name = n
        self.help = h
        self.arg = a
        self.options = []
        self.arrange_options = []
        self.longest_option_name = 0

    def add_option(self, option) :

        length = len("%s" % option)
        if length >= self.longest_option_name :
            self.longest_option_name = length
        self.options.append(option)

    def arrange(self) :

        self.arrange_options = []
        lengths = []
        for o in self.options :
            if len(o.name) not in lengths :
                lengths.append(len(o.name))
        lengths = sorted(lengths, key=int, reverse=True)
        for length in lengths :
            for o in self.options :
                if len(o.name) == length :
                    self.arrange_options.append(o)

    def print_options_count(self) :

        count = 0
        for o in self.options :
            if not o.hide :
                count += 1
        return count
        
def write_arg(file, cfile, arg) :

    cfile.write("const char * CFGArg_%s_HELP = R\"\"\"\"(\n" % arg.name.upper())
    if isinstance(arg.help, list) :
        for h in arg.help :
            cfile.write("%s\n" % h)
    else :
        cfile.write("%s\n" % arg.help)
    if arg.print_options_count() != 0 :
        cfile.write("\n  Explantion:\n")
        cfile.write("    <> : option/argument is required\n")
        cfile.write("    {} : option/argument is optional\n")
        cfile.write("    s  : option can be specified more than once\n")
        cfile.write("\n  Usage:\n")
        for o in arg.options :
            if not o.hide :
                if isinstance(o.help, str) :
                    cfile.write("    %-*s : %s\n" % (arg.longest_option_name, o, o.help))
                else :
                    cfile.write("    %-*s : %s\n" % (arg.longest_option_name, o, o.help[0]))
                    for hl in o.help[1:] :
                        cfile.write("    %s   %s\n" % (arg.longest_option_name * " ", hl))
                if o.type == "enum" :
                    cfile.write("    %-*s   Valid input is \"%s\"\n" % (arg.longest_option_name, \
                                                                        " ",
                                                                        "|".join(o.enum)))
        cfile.write("\n  Note: Use --help=option to show more detail of the option\n")
    cfile.write(")\"\"\"\";\n\n")
    file.write("class CFGArg_%s : public CFGArg\n" % arg.name.upper())
    file.write("{\n")
    file.write("public:\n")
    file.write("    CFGArg_%s();\n" % arg.name.upper())
    cfile.write("CFGArg_%s::CFGArg_%s() :\n" % (arg.name.upper(), arg.name.upper()))
    cfile.write("  CFGArg(\"%s\", %d, %d, {" % (arg.name, arg.arg[0], arg.arg[1]))
    if len(arg.options) == 0 :
        cfile.write("},\n")
    else :
        cfile.write("\n")
    arg.arrange()
    for i, o in enumerate(arg.arrange_options) :
        if isinstance(o.help, str) :
            hs = "\"%s\"" % o.help
        else :
            hs = ",".join(["\"%s\"" % hl for hl in o.help])
        cfile.write("    CFGArg_RULE(\"%s\", %s, \"%s\", %s, %s, %s, &%s, {%s}, %s)" % \
                                                (o.name,\
                                                 o.short_name,\
                                                 o.type,\
                                                 "true" if o.optional else "false",\
                                                 "true" if o.multiple else "false",\
                                                 "true" if o.hide else "false",\
                                                 o.name,\
                                                 hs,\
                                                 o.get_enum()))
        if i < (len(arg.options)-1) :
            cfile.write(",")
        else :
            cfile.write("},")
        cfile.write("\n")
    cfile.write("    CFGArg_%s_HELP)\n" % arg.name.upper())
    cfile.write("{\n")
    cfile.write("}\n\n")
    file.write("    bool parse(int argc, const char **argv, std::vector<std::string>* errors = nullptr) {\n")
    file.write("            return CFGArg::parse(argc, argv, errors); }\n")
    file.write("    void print() { CFGArg::print(); }\n")
    for o in arg.options :
        if o.type == "flag" :
            assert not o.multiple
            file.write("    bool %s = %s;\n" % (o.name, "true" if o.default else "false"))
        elif o.type == "int" :
            if o.multiple :
                if o.default == None :
                    file.write("    std::vector<uint64_t> %s = {};\n" % (o.name))
                else :
                    file.write("    std::vector<uint64_t> %s = {%s};\n" % (o.name, o.default))
            else :
                file.write("    uint64_t %s = %s;\n" % (o.name, o.default))
        elif o.type == "str" :
            if o.multiple :
                if o.default == None :
                    file.write("    std::vector<std::string> %s = {};\n" % (o.name))
                else :
                    file.write("    std::vector<std::string> %s = {\"%s\"};\n" % (o.name, o.default))
            else :
                file.write("    std::string %s = \"%s\";\n" % (o.name, o.default))
        else :
            file.write("    std::string %s = \"%s\";\n" % (o.name, o.default))
    file.write("};\n\n")

def main() :

    print("****************************************")
    print("*")
    print("* Auto generate CFG Args")
    print("*")
    assert len(sys.argv) >= 4, "CFGArg.py need input json file and output auto.h/auto.cpp arguments"
    assert os.path.exists(sys.argv[1]), "Input json file %s does not exist" % sys.argv[1]
    print("*    Input:      %s" % sys.argv[1])
    print("*    Output H:   %s" % sys.argv[2])
    print("*    Output CPP: %s" % sys.argv[3])
    print("*")
    print("****************************************")

    # Read the JSON file into args
    file = open(sys.argv[1], 'r')
    args = json.loads(file.read())
    file.close()
    hfilename = os.path.basename(sys.argv[2])
    filename = os.path.basename(sys.argv[1]).split('.')[0]

    # Open output file for writting
    file = open(sys.argv[2], 'w')
    cfile = open(sys.argv[3], 'w')
    file.write("#ifndef %s_AUTO_H\n" % filename)
    file.write("#define %s_AUTO_H\n\n" % filename)
    file.write("#include \"Configuration/CFGCommon/CFGArg.h\"\n")
    cfile.write("#include \"%s\"\n" % hfilename)
    file.write("\n/*\n")
    file.write("    This file is auto-generated by Configuration/CFGCommon/CFGArg.py\n")
    file.write("        Input: %s\n" % os.path.basename(sys.argv[1]))
    file.write("*/\n\n")
    cfile.write("\n/*\n")
    cfile.write("    This file is auto-generated by Configuration/CFGCommon/CFGArg.py\n")
    cfile.write("        Input: %s\n" % os.path.basename(sys.argv[1]))
    cfile.write("*/\n\n")

    # Do not allow duplicated definition
    names = []
    if len(sys.argv) > 4 :
        for otherfile in sys.argv[4:] :
            ofile = open(otherfile, 'r')
            otherargs = json.loads(ofile.read())
            ofile.close()
            for name in otherargs :
                assert name not in names
                names.append(name)
    for name in args :
        assert name not in names, "Duplicate: %s vs %s" % (name, names)
        names.append(name)
        assert "help" in args[name]
        help = args[name]["help"]
        arg = [0, 0]
        if "arg" in args[name] :
            arg = args[name]["arg"]
        db = database(name, help, arg)
        if "option" in args[name] :
            short_names = []
            long_names = []
            assert isinstance(args[name]["option"], list)
            for option in args[name]["option"] :
                assert isinstance(option, dict)
                assert "name" in option
                assert option["name"] != "help"
                db.add_option(OPTION(option, long_names, short_names))
        write_arg(file, cfile, db)

    # Close output file
    file.write("#endif\n")
    file.close()
    cfile.close()

if __name__ == "__main__":
    main()
