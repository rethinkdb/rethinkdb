module.exports = (function() {
    function opt(argv) {
        var opt={},arg,p;argv=Array.prototype.slice.call(argv||process.argv);for(var i=2;i<argv.length;i++)if(argv[i].charAt(0)=='-')
        ((p=(arg=(""+argv.splice(i--,1)).replace(/^[\-]+/,'')).indexOf("="))>0?opt[arg.substring(0,p)]=arg.substring(p+1):opt[arg]=true);
        return {'node':argv[0],'script':argv[1],'argv':argv.slice(2),'opt':opt};
    }
    return opt.opt = opt;
})();
