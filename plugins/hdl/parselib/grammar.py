from lark import Lark

lark_grammar = Lark('''
        start: modulelist typelist

        modulelist: (hmodule)*
        typelist: (htypedef)*
        // hmodule:  "hModule" ID "[" modportsiglist? (portbindinglist|processlist)* "]"
        hmodule:  "hModule" ID "[" modportsiglist? (hmodinitblock|processlist)* "]"

        modportsiglist: "hPortsigvarlist" "NONAME" ("[" modportsigdecl+ "]" | "NOLIST")

        ?modportsigdecl: portdecltype 
                      | sigdecltype 
                      | vardeclinit 
                      | moddecl
        moddecl: "hModdecl" ID "[" htypeinfo "]"
        portdecltype: portdecl "[" htypeinfo hvarinit? "]"
        sigdecltype: sigdecl "[" htypeinfo "]"
        sigdecl:  "hSigdecl" ID  
        ?portdecl: inportdecl | outportdecl
        inportdecl:  "hPortin" ID 
        outportdecl: "hPortout" ID
        vardeclinit: "hVardecl" ID "[" htypeinfo (hvarinit | hvarinitint)? "]"
        vardeclrn: "hVardeclrn" ID "[" htypeinfo hliteral "]"
        funcparami: "hFunctionParamI" ID "[" htypeinfo (hvarinit | hvarinitint)? "]"
        funcparamio: "hFunctionParamIO" ID "[" htypeinfo (hvarinit | hvarinitint)? "]"
        ?hvarinit: "hVarInit" "NONAME" expression
                 | "hVarInit" "NONAME" "[" hvarinitlist "]"
        ?hvarinitint: "hVarInit" NUM "NOLIST"
        // can be no process at all in the module
        processlist:  "hProcesses" "NONAME" "[" (hprocess|hfunction|hthread)*"]"
        // could be nothing
        // temporarily ignore the hMethod node
        hprocess: "hProcess" ID  "[" "hMethod" (ID|"NONAME") "[" prevardecl  hcstmt "]" "]"
        
        // hthread consists of a synchronous block that sets state and a switch block that produces next state
        // hfunction 
        hthread:  "hProcess" ID  "[" modportsiglist? hfunction* hthreadsync hthreadswitch "]"
        hthreadsync: "hMethod" ID  "[" ifstmt "]"
        hthreadswitch: "hMethod" ID  "["  stmts "]"
        
        prevardecl: vardecl*
        vardecl: vardeclinit | vardeclrn

        // can be just an empty statement
        hcstmt:  "hCStmt" "NONAME" "[" stmts "]" // useful for raising variable decls
              |  "hCStmt" "NONAME" "NOLIST"
        stmts: stmt+
        stmt : expression_in_stmt
             | syscwrite
             | ifstmt
             | forstmt
             | hcstmt
             | whilestmt
             | dostmt
             | switchstmt
             | blkassign
             | hnoop
             | hreturnstmt
             | breakstmt
             | continuestmt
             | hwait
             
        continuestmt: "hContinue" "NONAME" "NOLIST"
        // hvarinitlist can be empty
        hvarinitlist: "hVarInitList" "NONAME" "[" (hvarinitlist | expression)+ "]"
                    | "hVarInitList" "NONAME" "NOLIST"
        // hvarinitlist: "hVarInitList" "NONAME" "NOLIST"
        
        hwait: "hWait" "wait" "NOLIST" | "hWait" NUM ("[" hliteral "]" | "NOLIST")
             
        breakstmt: "hBreak" "NONAME" "NOLIST"
        
        ?htotype: htouint | htoint | htolong | htoulong | hnoop | htoi64 | htou64
             
        ?htobool: ("hBuiltinFunction" "to_bool" | "hNoop" "to_bool") "[" harrayref "]"
        htouint: "hBuiltinFunction" "to_uint" "[" (syscread|hvarref) "]"
        htoint: "hBuiltinFunction" "to_int" "[" (syscread|hvarref) "]"
        htolong: "hBuiltinFunction" "to_long" "[" (syscread|hvarref) "]"
        htoulong: "hBuiltinFunction" "to_ulong" "[" (syscread|hvarref) "]"
        hnoop: "hNoop" "NONAME" "NOLIST"
        htoi64: "hBuiltinFunction" "to_int64" "[" hvarref "]"
        htou64: "hBuiltinFunction" "to_uint64" "[" hvarref "]"
        
        // hmodinitblock: 
        // first component is the id of the module (in parent?)
        // second component is initialization list              
        // third component is port binding list
        hmodinitblock: "hModinitblock" ID "[" hcstmt* portbindinglist* hsenslist*"]"
                     | "hModinitblock" ID "NOLIST"

        // Port Bindings
        portbindinglist: "hPortbindings" ID "[" portbinding* "]"
        // hPortbinding u_dut [
        //   hVarref avg_out NOLIST
        //   hVarref dut_avg NOLIST
        // ]
        portbinding: "hPortbinding" ID "[" hvarref hvarref "]"
                   | "hPortbinding" ID "[" hbindingref hbindingref "]"
                   | "hPortbinding" ID "[" hvarref hbindingref "]"
                   | "hPortbinding" ID "[" hbindingarrayref hbindingarrayref "]"  
                   | "hPortbinding" ID "[" hvarref hbindingarrayref "]"  
                   // TODO: replace portbinding with succinct syntax
        hbindingref: "hVarref" ID "[" hliteral "]"
        // compared array ref in normal expressions
        // we use a more restrictive form here
        hbindingarrayref: "hBinop" "ARRAYSUBSCRIPT" "[" (hvarref|hbindingarrayref) (hliteral|hbinop) "]"  


        // This is solely for maintaining the semicolon
        expression_in_stmt: expression
        
        // while(condition) stmts
        whilestmt: "hWhileStmt" "NONAME" "[" expression stmt "]"
        
        // do stmts while(condition)
        dostmt: "hDoStmt" "NONAME" "[" expression stmt "]"

        // for(forinit; forcond; forpostcond) stmts
        forstmt: "hForStmt" "NONAME" "[" forinit forcond forpostcond forbody "]"
        forinit: "hPortsigvarlist" "NONAME" "[" vardeclinit  "]"
                 | vardeclinit
                 | hnoop
                 | blkassign
        forcond: expression
        forpostcond: expression
        forbody: stmt

        switchstmt: "hSwitchStmt" "NONAME" "[" switchcond switchbody "]"
        switchcond: expression
        // Note: we don't make this a noraml statement as in the context of switch, 
        // we don't use general statements
        switchbody: "hCStmt" "NONAME" "[" ((casestmt* breakstmt?)+) "]"
        casestmt: "hSwitchCase" "NONAME" "[" casevalue stmt+ "]" hnoop
                | "hSwitchCase" "NONAME" "[" casevalue hnoop "]"
                | "hSwitchCase" "NONAME" "[" casevalue stmt+ "]" 
                | "hSwitchDefault" "NONAME" "[" stmt+ "]"
        casevalue: expression
        
        // Function
        hfunction : "hFunction" ID "[" hfunctionrettype hfunctionparams? hfunctionlocalvars hfunctionbody "]"
        hfunctionlocalvars: vardeclinit*
        hfunctionbody: hcstmt
        hfunctionrettype: "hFunctionRetType" "NONAME" "[" htypeinfo "]"
                        | "hFunctionRetType" "NONAME" "NOLIST" // only appears in generated statements
        hfunctionparams : "hFunctionParams" "NONAME" "[" (funcparami|funcparamio)* "]"
        hreturnstmt: "hReturnStmt" "NONAME" "[" expression "]"
                   | "hReturnStmt" "NONAME" "NOLIST"  // return;

        hsenslist : "hSenslist" ID "[" hsensvar* "]"
                  | "hSenslist" ID "NOLIST"
        hsensvar :  "hSensvar" "NONAME" "[" (expression|hvalchange) ("hNoop" | "hBuiltinFunction")  npa "NOLIST" "]"
                 |  hasync
        hasync   :  "hSensvar" "ASYNC" "[" expression hliteral "]"

        hvalchange: "hNoop" "value_changed_event" "[" expression "]"
        hsensedge : "hNoop" npa "NOLIST"
                  | "hBuiltinFunction" npa "NOLIST"
        !npa : "neg" | "pos" | "always"

        // if and if-else, not handling if-elseif case
        ifstmt: "hIfStmt" "NONAME" "[" expression  stmt? stmt?"]"

         
        ?expression: hbinop
                  | hunop
                  | hliteral
                  | hvarref
                  | hunimp
                  | syscread
                  | hmethodcall
                  |  "[" expression "]"
                  | htobool
                  | htouint
                  | htoi64
                  | htou64
                  | htoint
                  | htolong
                  | htoulong
                  | hcondop
                  | hlrotate
                  | horreduce
                  | hfieldaccess
                  
        hfieldaccess: "hFieldaccess" "NONAME" "[" (harrayref|syscread) hfieldname "]"
        hfieldname:   "hField" ID "NOLIST"
                  
        hlrotate : "hBuiltinFunction" "lrotate" "[" expression expression "]"
        horreduce: "hBuiltinFunction" "or_reduce" "[" expression "]"
        hcondop : "hCondop" "NONAME" "[" (hslice | hliteral | hbinop | hunop | syscread | hvarref | hmethodcall) (hslice | expression | hprefix) (hslice | expression | hpostfix) "]"

        syscread : hsigassignr "[" (expression | harrayref) "]"
        syscwrite : hsigassignl "["  expression  (expression | hfieldaccess) "]"
        ?hsigassignr :  "hSigAssignR" "read" 
        ?hsigassignl :  "hSigAssignL" "write" 
        // function call
        hvarref : "hVarref" ID "NOLIST"
        hunimp:  "hUnimpl" ID "NOLIST"
        hbinop:  "hBinop" BINOP "[" (expression|hslice) (expression|hslice) "]"
        
        // A temporary hack to handle --
        hunop:  "hUnop" UNOP_NON_SUB "[" (expression|hslice) "]"
             |  "hUnop" UNOP_SUB "[" (expression|hslice) "]"
             |  "hUnop" UNOP_BNOT "[" (expression|hslice) "]"
             |  "hBinop" UNOP_NOT "[" (expression|hslice) "]"
             | hpostfix
             | hprefix
             | hunopdec
             | hreduceop
        hpostfix: "hPostfix" (UNOP_INC | UNOP_DEC) "[" expression "]"
        hprefix: "hPrefix" (UNOP_INC | UNOP_DEC) "[" expression "]"
        hunopdec: "hUnop" "-" "-" "[" expression "]" // hack to work with --
        
        hreduceop: "hNoop" REDUCE_OP "[" expression "]"
        REDUCE_OP: "and_reduce" | "or_reduce" | "xor_reduce" | "nand_reduce" | "nor_reduce" | "xnor_reduce"

        // Separate '=' out from so that it is not an expression but a standalone statement
        blkassign: "hBinop" "=" "[" (hconcat | hvarref | hliteral | hfieldaccess) (htotype | hconcat | hfieldaccess | hcomma | htobool | hunop | hvarref | hliteral | harrayref | hnsbinop | hunimp | syscread | hmethodcall | hcondop) "]"
                 | "hBinop" "=" "[" harrayref  arrayrhs "]"
                 | nblkassign
                 | vassign
                 | hmodassign
        // These assignments are only intended to be used as blocking assignments
        // The semantics may not be straightforward in clocked block
        hcompoundassign: "hBinop" COMPOUND_ASSIGN "[" hvarref hvarref "]"
        ?arrayrhs: horreduce 
                  | htobool 
                  | htoint 
                  | htouint 
                  | htolong 
                  | htoulong 
                  | hvarref 
                  | hliteral 
                  | harrayref 
                  | hnsbinop 
                  | hunimp 
                  | syscread 
                  | hmethodcall
                  | hunop
                  | hliteral
                  | hcondop
                  | htoint
                  
        nblkassign: "hSigAssignL" "write" "[" (hliteral | hvarref | harrayref) (syscread | hliteral | harrayref | hunop | hvarref | htobool | hmethodcall | hfieldaccess)  "]"
                  | "hSigAssignL" "write" "[" (hliteral | hvarref | harrayref) nonrefexp  "]"
        hconcat: ("hBinop" "concat" "[" | "hMethodCall" "NONAME" "[" "hBinop" "concat" "NOLIST") (expression|harrayref|hconcat) (expression|harrayref|hconcat) "]"
                 
        // Temporary hack to handle -= / +=
        hmodassign : "hBinop" hmodassigntype "[" hvarref (hliteral|hvarref) "]"
        ?hmodassigntype : haddassign | hsubassign
        haddassign : "+" "=" 
        hsubassign : "-" "="
        vassign: "hVarAssign" "NONAME" "[" hvarref (hnsbinop | syscread | hliteral | hvarref | expression | harrayref | hvarinitlist)"]"
        // Normal expressions that can not be expanded
        nonrefexp: hbinop
        
        harrayref: "hBinop" "ARRAYSUBSCRIPT"  "[" (hliteral | hvarref | syscread | harrayref) expression  "]"
                 | hslice
        hslice: "hBinop" "SLICE" "[" hvarref expression expression "]"
              | "hNoop" "range" "[" (hvarref | harrayref | syscread ) expression expression "]"
              | "hNoop" "bit" "[" (hvarref | harrayref | syscread) expression "]"
        hnsbinop:  "hBinop" NONSUBBINOP "[" (expression|hslice) (expression|hslice) "]"
        // Comma op is the C++ comma where the latter part of the comma expression is returned
        hcomma: "hBinop" "," "[" (blkassign | hunop | hmethodcall) (hunop | expression | hmethodcall) "]"

        hmethodcall: "hMethodCall" hidorstr  "[" expression expression* "]" 
                   | "hMethodCall" hidorstr  "NOLIST"
                   
        ?hidorstr: ID | STRING
        hliteral:  idlit | numlit | numlitwidth
        idlit : "hLiteral" ID "NOLIST"
        numlit : "hLiteral" NUM "NOLIST"
        numlitwidth : "hLiteral" NUM "[" htypeinfo "]"
        htypeinfo: "hTypeinfo" "NONAME" "[" htype "]"
                 | "hTypeinfo" "NONAME" "NOLIST" // ?
        htype: htypearray
             | "hType" TYPESTR "NOLIST" 
             | "hType" TYPESTR "[" (htype|htypeint)+ "]"     // nested types, type parameters
        hdeptype: "hType" "typename" TYPESTR "NOLIST"
        htypealias: "hTypeAlias" TYPESTR "[" htype "]" 
        htypealiases: htypealias* 
        htypearray : "hType" "array" arraydimlength "[" (htype|htypeint)+ "]"
        arraydimlength: "##" NUM 
                      | "##" NUM arraydimlength
        htypeint: "hLiteral" NUM "NOLIST"  // integer type parameters
        htypedef: "hTypedef" TYPESTR "[" htypetemplateparams htypealiases htypefields "]"
        
        htypetemplateparams: htypetemplateparam*
        htypetemplateparam: "hTypeTemplateParam" TYPESTR "NOLIST"
        
        htypefields: htypefield*
        htypefield: "hTypeField" ID "[" (htype|hdeptype) "]"

        ID: /[a-zA-Z_][a-zA-Z_0-9#]*/
        NUM: /(\+|\-)?[0-9]+/
        TYPESTR: /[a-zA-Z_]([a-zA-Z_0-9]|::)*/
        BINOP: COMPOUND_ASSIGN | NONSUBBINOP | "ARRAYSUBSCRIPT" | "SLICE" | "concat"
        NONSUBBINOP: "==" | "<<" | ">>" | "&&" | "||" | "|" | ">=" | ">" | ARITHOP | "<=" | "<" | "%" | "!=" | "&" | "@="
        ARITHOP: "+" | "-" | "*" | "/" | "^"
        UNOP_NON_SUB: "!" | "++" | "-" | "+"
        UNOP_SUB:  "-"
        UNOP_DEC:  "--"
        UNOP_INC:  "++"
        // These are temporary nodes that should be removed when hBinop is fixed
        UNOP_BOR: "|"
        UNOP_NOT: "!"
        UNOP_BNOT: "~"
        // alias_translation.py
        COMPOUND_ASSIGN: "*=" | "+=" | "-=" | "/=" | "%=" | "|=" | "&=" | "^=" | "<<=" | ">>="
        %import common.WS
        %ignore WS
        %import common.ESCAPED_STRING -> STRING
        ''', parser='lalr', debug=True, propagate_positions=True)


class UnexpectedHCodeStructureError(Exception):
    """raised when a hcode node is not as expected"""
    pass
