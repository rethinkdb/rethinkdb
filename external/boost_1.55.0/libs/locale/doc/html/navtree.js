var NAVTREE =
[
  [ "Boost.Locale", "index.html", [
    [ "Boost.Locale", "index.html", "index" ],
    [ "Modules", "modules.html", "modules" ],
    [ "Namespaces", null, [
      [ "Namespace List", "namespaces.html", "namespaces" ],
      [ "Namespace Members", "namespacemembers.html", [
        [ "All", "namespacemembers.html", "namespacemembers_dup" ],
        [ "Functions", "namespacemembers_func.html", "namespacemembers_func" ],
        [ "Variables", "namespacemembers_vars.html", null ],
        [ "Typedefs", "namespacemembers_type.html", null ],
        [ "Enumerations", "namespacemembers_enum.html", null ],
        [ "Enumerator", "namespacemembers_eval.html", null ]
      ] ]
    ] ],
    [ "Classes", null, [
      [ "Class List", "annotated.html", "annotated" ],
      [ "Class Index", "classes.html", null ],
      [ "Class Hierarchy", "hierarchy.html", "hierarchy" ],
      [ "Class Members", "functions.html", [
        [ "All", "functions.html", "functions_dup" ],
        [ "Functions", "functions_func.html", "functions_func" ],
        [ "Variables", "functions_vars.html", null ],
        [ "Typedefs", "functions_type.html", null ],
        [ "Enumerations", "functions_enum.html", null ],
        [ "Enumerator", "functions_eval.html", null ]
      ] ]
    ] ],
    [ "Files", null, [
      [ "File List", "files.html", "files" ]
    ] ],
    [ "Examples", "examples.html", "examples" ]
  ] ]
];

function getData(varName)
{
  var i = varName.lastIndexOf('/');
  var n = i>=0 ? varName.substring(i+1) : varName;
  return eval(n);
}

function stripPath(uri)
{
  return uri.substring(uri.lastIndexOf('/')+1);
}

function getScript(scriptName,func,show)
{
  var head = document.getElementsByTagName("head")[0]; 
  var script = document.createElement('script');
  script.id = scriptName;
  script.type = 'text/javascript';
  script.onload = func; 
  script.src = scriptName+'.js'; 
  script.onreadystatechange = function() {
    if (script.readyState == 'complete') { func(); if (show) showRoot(); }
  };
  head.appendChild(script); 
}

function createIndent(o,domNode,node,level)
{
  if (node.parentNode && node.parentNode.parentNode)
  {
    createIndent(o,domNode,node.parentNode,level+1);
  }
  var imgNode = document.createElement("img");
  imgNode.width = 16;
  imgNode.height = 22;
  if (level==0 && node.childrenData)
  {
    node.plus_img = imgNode;
    node.expandToggle = document.createElement("a");
    node.expandToggle.href = "javascript:void(0)";
    node.expandToggle.onclick = function() 
    {
      if (node.expanded) 
      {
        $(node.getChildrenUL()).slideUp("fast");
        if (node.isLast)
        {
          node.plus_img.src = node.relpath+"ftv2plastnode.png";
        }
        else
        {
          node.plus_img.src = node.relpath+"ftv2pnode.png";
        }
        node.expanded = false;
      } 
      else 
      {
        expandNode(o, node, false, false);
      }
    }
    node.expandToggle.appendChild(imgNode);
    domNode.appendChild(node.expandToggle);
  }
  else
  {
    domNode.appendChild(imgNode);
  }
  if (level==0)
  {
    if (node.isLast)
    {
      if (node.childrenData)
      {
        imgNode.src = node.relpath+"ftv2plastnode.png";
      }
      else
      {
        imgNode.src = node.relpath+"ftv2lastnode.png";
        domNode.appendChild(imgNode);
      }
    }
    else
    {
      if (node.childrenData)
      {
        imgNode.src = node.relpath+"ftv2pnode.png";
      }
      else
      {
        imgNode.src = node.relpath+"ftv2node.png";
        domNode.appendChild(imgNode);
      }
    }
  }
  else
  {
    if (node.isLast)
    {
      imgNode.src = node.relpath+"ftv2blank.png";
    }
    else
    {
      imgNode.src = node.relpath+"ftv2vertline.png";
    }
  }
  imgNode.border = "0";
}

function newNode(o, po, text, link, childrenData, lastNode)
{
  var node = new Object();
  node.children = Array();
  node.childrenData = childrenData;
  node.depth = po.depth + 1;
  node.relpath = po.relpath;
  node.isLast = lastNode;

  node.li = document.createElement("li");
  po.getChildrenUL().appendChild(node.li);
  node.parentNode = po;

  node.itemDiv = document.createElement("div");
  node.itemDiv.className = "item";

  node.labelSpan = document.createElement("span");
  node.labelSpan.className = "label";

  createIndent(o,node.itemDiv,node,0);
  node.itemDiv.appendChild(node.labelSpan);
  node.li.appendChild(node.itemDiv);

  var a = document.createElement("a");
  node.labelSpan.appendChild(a);
  node.label = document.createTextNode(text);
  node.expanded = false;
  a.appendChild(node.label);
  if (link) 
  {
    a.className = stripPath(link.replace('#',':'));
    if (link.indexOf('#')!=-1)
    {
      var aname = '#'+link.split('#')[1];
      var srcPage = stripPath($(location).attr('pathname'));
      var targetPage = stripPath(link.split('#')[0]);
      a.href = srcPage!=targetPage ? node.relpath+link : '#';
      a.onclick = function(){
        $('.item').removeClass('selected');
        $('.item').removeAttr('id');
        $(a).parent().parent().addClass('selected');
        $(a).parent().parent().attr('id','selected');
        var anchor = $(aname);
        $("#doc-content").animate({
          scrollTop: anchor.position().top +
          $('#doc-content').scrollTop() -
          $('#doc-content').offset().top
        },500,function(){
          window.location.replace(aname);
        });
      };
    }
    else
    {
      a.href = node.relpath+link;
    }
  } 
  else 
  {
    if (childrenData != null) 
    {
      a.className = "nolink";
      a.href = "javascript:void(0)";
      a.onclick = node.expandToggle.onclick;
    }
  }

  node.childrenUL = null;
  node.getChildrenUL = function() 
  {
    if (!node.childrenUL) 
    {
      node.childrenUL = document.createElement("ul");
      node.childrenUL.className = "children_ul";
      node.childrenUL.style.display = "none";
      node.li.appendChild(node.childrenUL);
    }
    return node.childrenUL;
  };

  return node;
}

function showRoot()
{
  var headerHeight = $("#top").height();
  var footerHeight = $("#nav-path").height();
  var windowHeight = $(window).height() - headerHeight - footerHeight;
  (function (){ // retry until we can scroll to the selected item
    try {
      navtree.scrollTo('#selected',0,{offset:-windowHeight/2});
    } catch (err) {
      setTimeout(arguments.callee, 0);
    }
  })();
}

function expandNode(o, node, imm, showRoot)
{
  if (node.childrenData && !node.expanded) 
  {
    if (typeof(node.childrenData)==='string')
    {
      var varName    = node.childrenData;
      getScript(node.relpath+varName,function(){
        node.childrenData = getData(varName);
        expandNode(o, node, imm, showRoot);
      }, showRoot);
    }
    else
    {
      if (!node.childrenVisited) 
      {
        getNode(o, node);
      }
      if (imm)
      {
        $(node.getChildrenUL()).show();
      } 
      else 
      {
        $(node.getChildrenUL()).slideDown("fast");
      }
      if (node.isLast)
      {
        node.plus_img.src = node.relpath+"ftv2mlastnode.png";
      }
      else
      {
        node.plus_img.src = node.relpath+"ftv2mnode.png";
      }
      node.expanded = true;
    }
  }
}

function showNode(o, node, index)
{
  if (node.childrenData && !node.expanded) 
  {
    if (typeof(node.childrenData)==='string')
    {
      var varName    = node.childrenData;
      getScript(node.relpath+varName,function(){
        node.childrenData = getData(varName);
        showNode(o,node,index);
      },true);
    }
    else
    {
      if (!node.childrenVisited) 
      {
        getNode(o, node);
      }
      $(node.getChildrenUL()).show();
      if (node.isLast)
      {
        node.plus_img.src = node.relpath+"ftv2mlastnode.png";
      }
      else
      {
        node.plus_img.src = node.relpath+"ftv2mnode.png";
      }
      node.expanded = true;
      var n = node.children[o.breadcrumbs[index]];
      if (index+1<o.breadcrumbs.length)
      {
        showNode(o,n,index+1);
      }
      else
      {
        if (typeof(n.childrenData)==='string')
        {
          var varName = n.childrenData;
          getScript(n.relpath+varName,function(){
            n.childrenData = getData(varName);
            node.expanded=false;
            showNode(o,node,index); // retry with child node expanded
          },true);
        }
        else
        {
          if (o.toroot=="index.html" || n.childrenData)
          {
            expandNode(o, n, true, true);
          }
          var a;
          if ($(location).attr('hash'))
          {
            var link=stripPath($(location).attr('pathname'))+':'+
                     $(location).attr('hash').substring(1);
            a=$('.item a[class*=\""'+link+'"\"]');
          }
          if (a && a.length)
          {
            a.parent().parent().addClass('selected');
            a.parent().parent().attr('id','selected');
            var anchor = $($(location).attr('hash'));
            var targetDiv = anchor.next();
            $(targetDiv).children('.memproto,.memdoc').
                     effect("highlight", {}, 1500);
          }
          else
          {
            $(n.itemDiv).addClass('selected');
            $(n.itemDiv).attr('id','selected');
          }
        }
      }
    }
  }
}

function getNode(o, po)
{
  po.childrenVisited = true;
  var l = po.childrenData.length-1;
  for (var i in po.childrenData) 
  {
    var nodeData = po.childrenData[i];
    po.children[i] = newNode(o, po, nodeData[0], nodeData[1], nodeData[2],
      i==l);
  }
}

function initNavTree(toroot,relpath)
{
  var o = new Object();
  o.toroot = toroot;
  o.node = new Object();
  o.node.li = document.getElementById("nav-tree-contents");
  o.node.childrenData = NAVTREE;
  o.node.children = new Array();
  o.node.childrenUL = document.createElement("ul");
  o.node.getChildrenUL = function() { return o.node.childrenUL; };
  o.node.li.appendChild(o.node.childrenUL);
  o.node.depth = 0;
  o.node.relpath = relpath;
  o.node.expanded = false;
  o.node.isLast = true;
  o.node.plus_img = document.createElement("img");
  o.node.plus_img.src = relpath+"ftv2pnode.png";
  o.node.plus_img.width = 16;
  o.node.plus_img.height = 22;

  getScript(relpath+"navtreeindex",function(){
    var navTreeIndex = eval('NAVTREEINDEX');
    if (navTreeIndex) {
      o.breadcrumbs = navTreeIndex[toroot];
      if (o.breadcrumbs==null) o.breadcrumbs = navTreeIndex["index.html"];
      o.breadcrumbs.unshift(0);
      showNode(o, o.node, 0);
    }
  },true);

  $(window).bind('hashchange', function(){
     if (window.location.hash && window.location.hash.length>1){
       var anchor = $(window.location.hash);
       var targetDiv = anchor.next();
       $(targetDiv).children('.memproto,.memdoc').effect("highlight",{},1500);
       var docContent = $('#doc-content');
       if (docContent && anchor && anchor[0] && anchor[0].ownerDocument){
         docContent.scrollTop(anchor.position().top+docContent.scrollTop()-docContent.offset().top);
       }
       var a;
       if ($(location).attr('hash')){
         var link=stripPath($(location).attr('pathname'))+':'+
                  $(location).attr('hash').substring(1);
         a=$('.item a[class*=\""'+link+'"\"]');
       }
       if (a && a.length){
         $('.item').removeClass('selected');
         $('.item').removeAttr('id');
         a.parent().parent().addClass('selected');
         a.parent().parent().attr('id','selected');
         var anchor = $($(location).attr('hash'));
         var targetDiv = anchor.next();
         showRoot();
       }
     } else {
       var docContent = $('#doc-content');
       if (docContent){ docContent.scrollTop(0); }
     }
  })

  $(window).load(showRoot);
}

