// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

//--------------Event-handlers------------------------------------------------//

function toggle(id) { get_tree().find(id).toggle(); }
function blur_tree() 
{ 
    if (  window.event && 
          window.event.srcElement && 
          window.event.srcElement.blur &&
          window.event.srcElement != document.body  )
        window.event.srcElement.blur();
    else if (target_frame()) 
        target_frame().window.focus(); 
}
document.onclick = blur_tree;

//--------------Netscape 4.x-specific-----------------------------------------//

window.saved_width = window.outerWidth;
function reload_tree() 
{   
    if (window.outerWidth != window.saved_width) 
        window.location.reload();
}
if (window.Event && Event.RESIZE) {
    window.captureEvents(Event.RESIZE);
    window.onResize = reload_tree;
}

//--------------Functions for browser-sniffing--------------------------------//

function major_version(app)
{
    var index = navigator.userAgent.indexOf(app);
    if (index == -1)
        return -1;
    return parseInt(navigator.userAgent.charAt(index + app.length + 1));
}
function dom_support()
{
    if (dom_support.cache == null)
        dom_support.cache = dom_support_impl();
    return dom_support.cache;
}
function dom_support_impl()
{
    var version;
    if ( (version = major_version("Mozilla")) != -1 &&
         navigator.userAgent.indexOf("compatible") == -1 ) 
        return version > 4;
    if ((version = major_version("Opera")) != -1) 
        return version > 6;
    if ((version = major_version("Konqueror")) != -1) 
        return version > 2;
    if ((version = major_version("Links")) != -1) 
        return false;
    return document.getElementById || document.all;
}

//--------------Utility functions---------------------------------------------//

function target_frame() 
{ 
    return get_tree() ? top.frames[get_tree().target] : null; 
}
function get_tree() { return document.tree_control; }
function static_display() { return !dom_support() || get_tree().dump_html; }
function elt_by_id(id)
{
    return document.all ?
        document.all[id] :
        document.getElementById ?
            document.getElementById(id) :
            null;
}
function replace_query(url, query)
{
    var pos;
    if ((pos = url.indexOf("?")) != -1)
        url = url.substring(0, pos);
    return url + "?" + query;
}

//--------------Functions for HTML-generation---------------------------------//

function icon_char(state)
{
    return state == tree_node.expanded ?
                "-" :
                state == tree_node.collapsed ?
                    "+" :
                    "&nbsp;";
}
function html_list(id, display, margin)
{ 
    return "<div id='" + id + "' style='white-space:nowrap;display:" + 
           display + "'>";
}
function html_list_item(content)
{ 
    return "\n<div class='tree-item'>" + content + "</div>"; 
}
function html_anchor(content, cl, href, target)
{ 
    return "<a class='" + cl + "' onfocus='blur_tree()" + 
           "' href='" +  href + "'" + 
           (target ? " target='" + target + "'" : "") +
           ">" + content + "</a>"; 
}

//--------------Definition of class tree_node---------------------------------//

function tree_node__add(text_or_node, link_or_hide, hide) 
{     
    if (this.state == tree_node.neutral)
        this.state = tree_node.collapsed;
    var k;
    if (text_or_node.length != null) {
        k = new tree_node(text_or_node, link_or_hide);
        k.hide_kids = hide != null ? hide : false;
    } else {
        k = text_or_node;
        k.hide_kids = link_or_hide != null ? link_or_hide : false;
    }
    k.mom = this;
    if (this.kids == null)
        this.kids = new Array();
    this.kids[this.kids.length] = k; 
    return k;
}
function tree_node__level() 
{ 
    var level;
    var node;
    for (node = this.mom, level = -1; node; node = node.mom, ++level)
        ;
    return level;
}
function tree_node__parent() { return this.mom; }
function tree_node__print() 
{
    var icon = 
        !static_display() ?  
            "<span style='font-family:monospace' class='tree-icon' id='icon" + 
                 this.id + "'>" + icon_char(this.state) + "</span>&nbsp;&nbsp;" :
            "";
    var handler = 
        !static_display() && this.kids ?
            "javascript:toggle(\"id" + this.id + "\")" :
            "";
    var text = "<span class='tree-text'>" + this.text + "</span>"
    var tree = get_tree();
    var indent = tree.indent * this.level();
    return html_list_item(
               "<div style='margin-left:" + (2 * indent) + 
               ";text-indent:-" + indent + "'>" +
               ( !tree.dump_html ?
                     this.kids ?
                         html_anchor(icon, "tree-icon", handler) :
                         icon :
                     "" ) +
               ( tree.numbered ? 
                     "" + "<span class='tree-label'>" + 
                         this.id.substring(1) + "</span>" : 
                     "" ) +
               "&nbsp;&nbsp;" +
               ( this.link ?
                     html_anchor( text, "tree-text", this.link, 
                                  tree.target ) :
                     text ) + 
               "</div>" + 
               this.print_kids()
           );
}
function tree_node__print_kids(margin) 
{
    var result = "";
    if (this.kids != null && (!static_display() || !this.hide_kids)) {
        if (margin == null)
            margin = get_tree().indent;
        result += html_list( "list" + this.id,
                             this.state == tree_node.collapsed && 
                             !static_display() 
                                ? "none" : "",
                             margin );
        for (var z = 0; z < this.kids.length; ++z) {
            var k = this.kids[z];
            k.id = this.id + "." + (z + 1);
            result += k.print();
        }
        result += "</div>";
    }
    return result;
}
function tree_node__toggle(expand) 
{
    if ( static_display() ||
         this.kids == null || 
         expand != null && expand == 
            (this.state == tree_node.expanded) )
    {
        return;
    }
    this.state = 
        this.state == tree_node.expanded ?
            tree_node.collapsed :
            tree_node.expanded;
    elt_by_id("icon" + this.id).innerHTML = 
        icon_char(this.state);
    elt_by_id("list" + this.id).style.display = 
        this.state == tree_node.expanded ?
            "" : 
            "none";
}
function add_methods(obj)
{
    obj.add = tree_node__add;
    obj.level = tree_node__level;
    obj.parent = tree_node__parent;
    obj.print = tree_node__print;
    obj.print_kids = tree_node__print_kids;
    obj.toggle = tree_node__toggle;
}
function tree_node(text, link) 
{
    // Member data
    this.text = text;
    this.link = link;
    this.mom = null;
    this.kids = null;
    this.id = null;
    this.state = 0; // Neutral.

    if (!add_methods.prototype)
        add_methods(this);
}
tree_node.neutral = 0;
tree_node.expanded = 1;
tree_node.collapsed = 2;
if (tree_node.prototype)
    add_methods(tree_node.prototype);
                
//--------------Definition of class tree_control------------------------------//

function tree_control__add(text, link, hide) 
{ 
    return this.root.add(text, link, hide); 
}
function tree_control__draw() 
{ 
    var tree    = get_tree();
    var dom     = dom_support();
    var element = dom ? elt_by_id('tree_control') : null;
    if (element || !dom || tree.drawn) {
        var html = tree.html();
        if (tree.dump_html) {
            var pat = new RegExp("<", "g");
            html = "<pre>" + html.replace(pat, "&lt;") + "</pre>";
            if (document.body.innerHTML)
                document.body.innerHTML = html;
            else 
                document.write(html);
        } else if (dom) {
            element.innerHTML = html;
        } else {
            document.open();
            document.write( 
                "<body>" + html +
                ( major_version("MSIE") == 3 ?
                     "<noscript>" :
                     document.layers ?
                         "<layer visibility='hide'>" :
                         "<table width=0 height=0 style='" + 
                         "visibility:hidden;display:none'><tr><td>" ) 
            );
            document.close();
        } 
        tree.drawn = true;
        tree.load();
    } else { 
        var t = navigator.userAgent.indexOf("Clue") != -1 ? 500 : 100;
        setTimeout("tree_control__draw()", t); 
    }
}
function tree_control__find(id) 
{ 
    var indices = id.split(".");
    var node = this.root;
    for (var z = 1; z < indices.length; ++z) 
        node = node.kids[indices[z] - 1];
    return node;
}
function tree_control__html()
{
    return  "<table><tr><td align='left'><table width=150><tr><td>" +
            "<h1 class=tree-caption>" + this.caption + "</h1></td></tr>" +
            ( !static_display() ? 
                  "<tr><td><p class='tree-sync'><a title='reload current " + 
                      "page with a url suitable for bookmarking' class=" +
                      "'tree-sync' href='javascript:get_tree().sync()'>" + 
                      "[link to this page]</a></p></td></tr>" :
                  "" ) + 
            "</table></td></tr><tr><td>" + this.root.print_kids(0) + 
            "</td></tr></table>";
}
function load_target(url)
{
    var target;
    if ((target = target_frame()) && target.location.href != url)
        target.location.replace(url);
    else {
        setTimeout("load_target('" + url + "')", 100);
    }
}
function tree_control__load() 
{ 
    var query;
    if ((query = top.location.search).length == 0) 
        return;
    query = query.substring(1);
    var eq;
    if ((eq = query.indexOf("=")) != -1) {
        if (query.substring(0, 4) == "page") {
            load_target(unescape(query.substring(eq + 1)));
            return;
        }
        query = query.substring(eq + 1);
    }
    var indices = query.split(".");
    if (!indices.length)
        return;
    this.reset();
    var node = this.root;
    for (var z = 0; z < indices.length; ++z) {
        var i = parseInt(indices[z]) - 1;
        if (!node.kids || i < 0 || node.kids.length <= i)
            break;
        node = node.kids[i];
        node.toggle(/*z != indices.length - 1*/);
    }
    if (node.link)
        load_target(node.link);
}
function tree_control__recurse(op) 
{
    var stack = new Array();
    stack[stack.length] = this.root;
    while (stack.length) {
        var node = stack[stack.length - 1];
        stack.length -=1 ; // Konqueror 2.
        op(node);
        if (node.kids)
            for (var z = 0; z < node.kids.length; ++z)
                stack[stack.length] = node.kids[z];
    }
}
function tree_control__reset() 
{ 
    if (!dom_support())
        return;
    this.recurse(new Function("x", "if (x.parent()) x.toggle(false);"));
}
function sync_node(node)
{
    if (!node.link)
        return;
    var tgt = target_frame().location.href;
    var pos;
    if ((pos = tgt.indexOf("?")) != -1)
        tgt = tgt.substring(0, pos);
    if (node.link.indexOf("://") != -1) {
        if (node.link != tgt) 
            return;
    } else {
        var base = window.location.href;
        if ((pos = base.lastIndexOf("/")) != -1)
            base = base.substring(0, pos + 1);
        if (base + node.link != tgt)
            return;
    }
    window.success = true;
    var href = replace_query( get_tree().top_url,
                              "path=" + node.id.substring(1) );
    top.location.replace(href);
}
function tree_control__sync() 
{ 
    if (!dom_support() || self == top)
        return;
    window.success = false;
    get_tree().recurse(sync_node);
    if (!window.success)
        top.location.replace( 
            replace_query( get_tree().top_url,
                           "page=" + escape(target_frame().location.href) )
        );
}
function tree_control(target) 
{
    // Member data
    this.root = new tree_node("");
    this.target = target ? target : "_self";
    this.dump_html = false;
    this.caption = "Contents";
    this.numbered = true;
    this.indent = 15;
    this.drawn = false;
    this.top_url = top.location.href; // For Opera.

    this.root.state = tree_node.expanded;
    this.root.id = "";

    // Member functions
    this.add = tree_control__add;
    this.draw = tree_control__draw;
    this.find = tree_control__find;
    this.html = tree_control__html;
    this.load = tree_control__load;
    this.recurse = tree_control__recurse;
    this.reset = tree_control__reset;
    this.sync = tree_control__sync;
    document.tree_control = this;
}
tree_control.sync = tree_control__sync;