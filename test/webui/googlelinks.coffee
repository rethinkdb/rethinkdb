links = []
casper = require("casper").create()

getLinks = ->
    links = document.querySelectorAll("h3.r a")
    Array::map.call links, (e) ->
        try
            (/url\?q=(.*)&sa=U/).exec(e.getAttribute("href"))[1]
        catch e
            e.getAttribute "href"

casper.start "http://google.fr/", ->
    # search for 'casperjs' from google form
    @fill "form[action=\"/search\"]", q: "casperjs", true

casper.then ->
    # aggregate results for the 'casperjs' search
    links = @evaluate(getLinks)
    # now search for 'phantomjs' by fillin the form again
    @fill "form[action=\"/search\"]", q: "phantomjs", true

casper.then ->
    # aggregate results for the 'phantomjs' search
    links = links.concat(@evaluate(getLinks))

casper.run ->
    # echo results in some pretty fashion
    @echo links.length + " links found:"
    @echo " - " + links.join("\n - ")
    @exit()
