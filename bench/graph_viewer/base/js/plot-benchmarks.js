// Copyright 2010-2012 RethinkDB, all rights reserved.
var plot_bm = {
	data: {},
	default_sets: ['rethinkdb'],
	default_series: ['qps'],
	default_options: ['enableTooltip'],
	// Assign specific colors to a given series
	colors: {},
	// Use the given set of colors to for all the series in incremental order
	color_scheme: [],
	filename_date_format: 'ddd MMM d HH:mm:ss yyyy',
	formatted_date: 'MMM d, yyyy - h:mm tt',
	date_latest_filename: 'latest.js',
	date_latest: 'Latest',
	all_data: '*',
	current_script: '',
	current_test: '',
	current_date: '',
	large_series_warning: 1000,
	axis_list: [],
	tools: {
		zoom: {
			current_mode: 0,
			modes: ['xy','x','y'],
			active: true,
		},
		pan: {
			active: false,
		}
	},
	units: []
};

var divs = {};

var formatted_dates = {};

$(document).ready(function() {
	divs.header = $('#header');
	divs.controls = $('#controls');
	divs.placeholder = $('#placeholder');	
	divs.legend = $('#legend');
	divs.overview = $('#overview');
	divs.time = $('#time');
	divs.footer = $('#footer');
	
	plot_bm.options = {
		series:  {
			shadowSize: 0,
		},
		grid: {
			clickable: true,
			hoverable: true
		},
		lines: { show: true },
		points: { show: true },
		selection: { mode: plot_bm.tools.zoom.current_mode },
		zoom: {	interactive: true },
		pan: { interactive: false },
		
		xaxes: [],
		yaxes: [],
	};
	
	plot_bm.build_controls();
	plot_bm.assign_plot_actions();
	plot_bm.assign_overview_actions();
	plot_bm.assign_option_actions();
	
	// Determine which file we are being asked to load
	get_data(window.location.hash.slice(1));
});

// Get the data from the specified URL
var get_data = function(url) {
	$('#loadingAnimation').remove();
	$('#placeholder').animate({opacity: 0.2}, 1000);
	$('<img id="loadingAnimation" src="base/images/loading-animation.gif" />').appendTo("#plot");
	$.getJSON(url, function(json) {
		plot_bm.current_date = json.date;
	    plot_bm.metadata = json.metadata;
	    plot_bm.data = json.data;
	    
        plot_bm.build_controls();
        $('#loadingAnimation').animate( {opacity: 0.0}, 1000, function() {
            $(this).remove();
            $('#placeholder').animate({opacity: 1.0}, 1000);

            var plot_title = $('#plotTitle').text(url);
        });
    });
};

// Plot an empty graph when the page loads
$(window).load(function() {
	plot_bm.replot();
});

// Replot the current plot
plot_bm.replot = function() {
	plot_bm.active_data = [];
	plot_bm.units = [];
	var datetime_format = '';
	
	if (plot_bm.current_date != '') {
		parsedDate = Date.parseExact(plot_bm.current_date, plot_bm.filename_date_format);
		//console.log("date: "+parsedDate);
		if (parsedDate != null)
			datetime_format = 'Benchmarks generated on '+Date.parseExact(plot_bm.current_date, plot_bm.filename_date_format).toString(plot_bm.formatted_date)+'.';
	}
	
	// Build the units list
	$('.set:checked').each(function(key,val) {
		set_name = $(this).attr('name');
		$('.series:checked').each(function(key,val) {
			series_name = $(this).attr('name');
			series = plot_bm.data[set_name][series_name];
			
			if (series != null && series.unit != "" && $.inArray(series.unit,plot_bm.units) < 0)
				plot_bm.units.push(series.unit);
		});
	});
	
	// Collect the active data based on the user's selections
	$('.set:checked').each(function(key,val) {
		set_name = $(this).attr('name');
		var y_axis_num = plot_bm.units.length+1;
		
		$('.series:checked').each(function(key, val) {
			series_name = $(this).attr('name');
			series_id = $(this).attr('id');
			series = plot_bm.data[set_name][series_name];
			
			if (series != null) {
				series_unit_num = $.inArray(series.unit,plot_bm.units);
				var test = {
					id: parseInt(series_id),
					color: (plot_bm.color_scheme.length > 0) ? plot_bm.color_scheme[parseInt(series_id) % plot_bm.color_scheme.length] : parseInt(series_id),
					label : set_name + " : " + series_name,
					data : series.data,
					yaxis: (series_unit_num > -1) ? series_unit_num+1 : y_axis_num
				};
				plot_bm.active_data.push(test);
				
				if (series_unit_num < 0)
					y_axis_num = y_axis_num + 1;
			}
		});
	});

	// Update the date/timestamp
	divs.time.text(datetime_format);

	// Make the plot's height fill the window
	var h = $(window).height() - divs.header.outerHeight() - divs.footer.outerHeight() - divs.placeholder.css('margin-top').replace('px','') - divs.placeholder.css('margin-bottom').replace('px','');
	divs.placeholder.height(h);
	
	// Set zooming or panning mode based on what's active	
	if (plot_bm.tools.zoom.active) {
		plot_bm.options.selection.mode = plot_bm.tools.zoom.modes[plot_bm.tools.zoom.current_mode];
		plot_bm.options.pan.interactive = false;
	} else if (plot_bm.tools.pan.active) {
		plot_bm.options.selection.mode = null;
		plot_bm.options.pan.interactive = true;
	}
	
	
	// Draw the plot
	plot_bm.plot = $.plot(divs.placeholder, plot_bm.active_data, plot_bm.options);
	
	// Draw the plot overview
	plot_bm.overview = $.plot(divs.overview, plot_bm.active_data, {
		legend: { show: false },
		lines: { show: true, lineWidth: 1 },
		shadowSize: 0,
		xaxis: { ticks: [] },
		yaxis: { ticks: [] },
		grid: { color: "#999", clickable: true },
		selection: { mode: "x" }
	});
	
	// Add navigation controls
	plot_bm.add_nav_controls();
	
	// Add axis highlighting
	plot_bm.add_axis_highlighting();
};

// Build the controls for the plot
plot_bm.build_controls = function() {
	var data_set_labels = [];
	var data_series_labels = [];
	var large_data_series = [];
	
	$('#chooseSets').empty();
	$('#chooseSeries').empty();

	
	// Loop through all sets and series in the dataset and collect labels for each in appropriate arrays
	$.each(plot_bm.data, function(key, set) {
		data_set_labels.push(key);
		$.each(set, function(key, series) {
			if ($.inArray(key,data_series_labels) < 0 ) {
				data_series_labels.push(key);
			}
			// Check if the series is gigantic
			if (series.data.length > plot_bm.large_series_warning)
				large_data_series.push(key);
		});
	});
	
	// Sort the sets and create their checkboxes
	$.each(data_set_labels.sort(), function(i, set) {
		//series.color = colors[series.label];
		$('<p class="controlOption"><input class="set" name="'+set+'" type="checkbox">'+set+'</input></p>').appendTo('#chooseSets');
	});
	
	// Sort the series and create their checkboxes
	$.each(data_series_labels.sort(), function(i, series) {
		$('<p class="controlOption"><input id="'+i+'" class="series" name="'+series+'" type="checkbox">'+series+'</input></p>').appendTo('#chooseSeries');

		// If the series is gigantic, add a class so we can throw up an appropriate warning before plotting
		if ($.inArray(series,large_data_series) > -1) {
			$('.series[name^='+series+']').addClass('largeData');		
		}
	});

	// Update the plot whenever a set is selected or deselected
	$('.set').click(function() {
		plot_bm.replot();
	}); 

	// Update the plot whenever a series is selected or deselected.
	$('.series').click(function() {
		// Check if the selected data series is gigantic, and throw up a warning before plotting the series
		if ($(this).hasClass('largeData') && $(this).attr('checked')) {
			var answer = confirm("The data set '" + $(this).attr('name') + "' has over 1,000 data points, which will take a very long time to plot. Proceed with plotting?");
			if (answer < 1) {
				$(this).attr('checked',false);
				return;
			}
		}
		
		// Reset the plot selection zoom		
		plot_bm.reset_axis_ranges();
		
		plot_bm.replot();
	});
	
	// Make sure the default sets are checked
	$.each(plot_bm.default_sets, function() {
		$('.set[name^='+this+']').click();
	});
	
	// Make sure the default series are checked
	$.each(plot_bm.default_series, function() {
		$('.series[name^='+this+']').click();
	});

	// Set the default state of each control section (collapsed / expanded)
	$('#chooseSetsHeader').jcollapser({target: '#chooseSets', state: 'collapsed'});	
	$('#chooseSeriesHeader').jcollapser({target: '#chooseSeries', state: 'expanded'});	
	$('#plotOptionsHeader').jcollapser({target: '#plotOptions', state: 'collapsed'});	
	$('#overviewHeader').jcollapser({target: '#overview', state: 'collapsed'});	
	
	// DIRTY HACK: Force a replot and reset the axis range to ensure *all* default series are plotted (click handler replots each series before the input box reports it's been selectedon page load);
	plot_bm.replot();
	plot_bm.reset_axis_ranges();
};

// Add axis highlighting to the plot
plot_bm.add_axis_highlighting = function() {
	plot_bm.axis_list = [];
	// For each y-axis, create a div (taken from Flot examples)
	if (plot_bm.active_data.length > 0) {
		$.each(plot_bm.plot.getData(), function(i, data) {
			var axis = data.yaxis;
			var box = getBoundingBoxForAxis(plot_bm.plot, axis);
			var axis_elem = $('<div class="axisTarget ' + data.id + '" style="position:absolute; left:' + box.left + 'px; top:' + box.top + 'px; width: ' + box.width + 'px; height: ' + box.height + 'px"</div>')
				.data('axis.direction', axis.direction)
				.data('axis.n', axis.n)
				.css({backgroundColor: data.color, opacity: 0, cursor: "pointer", border: "thin solid #000"})
				.appendTo(plot_bm.plot.getPlaceholder())
				.hover(
					function() {
						$(this).css({opacity: 0.10})	
					},
					function() {
						$(this).css({opacity: 0})	
					}
				)
				.click(function() {
					console.log("You clicked the " + axis.direction + axis.n + " axis.");
				});
			plot_bm.axis_list[data.id] = axis_elem;
		});
	}
		
	// Helper function: get the bounding box for each axis (taken from Flot examples)
	function getBoundingBoxForAxis(plot, axis) {
		var left = axis.box.left;
		var top = axis.box.top;
		var right = left+axis.box.width;
		var bottom = top+axis.box.height;
		
		// Expand the box to incorporate all of the axis' ticks if necessary
        var cls = axis.direction + axis.n + 'Axis';
        plot.getPlaceholder().find('.' + cls + ' .tickLabel').each(function () {
            var pos = $(this).position();
            left = Math.min(pos.left, left);
            top = Math.min(pos.top, top);
            right = Math.max(Math.round(pos.left) + $(this).outerWidth(), right);
            bottom = Math.max(Math.round(pos.top) + $(this).outerHeight(), bottom);
		});
		
		return {left: left, top: top, width: right - left, height: bottom - top};
	}
}

// Add navigation controls to the plot
plot_bm.add_nav_controls = function() {
	// Navigation controls: add zoom out button (taken from Flot examples)
    $('<div id="reset" class="button" style="position: absolute;right:20px;bottom:30px;color: #777; font-size: small;">reset zoom and pan</div>').appendTo(divs.placeholder).click(function (e) {
        e.preventDefault();
        
        // Reset the plot selection zoom		
		plot_bm.reset_axis_ranges();
		
       plot_bm.replot();
    });
    
	// Navigation controls: Helper function for adding panning arrows    
    function addArrow(dir, right, bottom, offset) {
        $('<img class="button" src="base/images/arrow-' + dir + '.gif" style="position: absolute;right:' + right + 'px;bottom:' + bottom + 'px">').appendTo(divs.placeholder).click(function (e) {
            e.preventDefault();
            plot_bm.plot.pan(offset);
        });
    }
    
	// Navigation controls: add panning buttons (taken from Flot examples)
    addArrow('left', 55, 70, { left: -100 });
    addArrow('right', 25, 70, { left: 100 });
    addArrow('up', 40, 85, { top: -100 });
    addArrow('down', 40, 55, { top: 100 });
    
    // Navigation controls: add panning button
    $('<img class="button" id="pan" src="base/images/pan.png" style="position: absolute;right:32px;bottom: 120px;" />').appendTo(divs.placeholder).click(function (e) {
		e.preventDefault();
		if(!plot_bm.tools.pan.active) {
			// Turn panning on
			$(this).addClass('selected').attr('src','base/images/pan_selected.png');
			plot_bm.tools.pan.active = true;
			
			// Deactivate zooming
			if(plot_bm.tools.zoom.active) {
				$('#zoom').removeClass('selected').attr('src','base/images/zoom.png');
				plot_bm.tools.zoom.active = false;
			}
			
			// Save the axis ranges between tools for continuity
			plot_bm.save_axis_ranges();
			
			plot_bm.replot();
		}
	});
        
    // Navigation controls: add zooming button
     $('<img class="button" id="zoom" src="base/images/zoom.png" style="position: absolute;right:11px;bottom: 170px;" />').appendTo(divs.placeholder).click(function (e) {
        e.preventDefault();
        if(!plot_bm.tools.zoom.active) {
        	// Turn zooming on
        	$(this).addClass('selected').attr('src','base/images/zoom_selected_'+plot_bm.tools.zoom.modes[plot_bm.tools.zoom.current_mode]+'.png');
        	plot_bm.tools.zoom.active = true;
        	
        	// Deactivate panning
        	if (plot_bm.tools.pan.active) {
				$('#pan').removeClass('selected').attr('src','base/images/pan.png');
				plot_bm.tools.pan.active = false;
			}
			
			// Save the axis ranges between tools for continuity
			plot_bm.save_axis_ranges();
			
			plot_bm.replot();
        }
        // If zoom is already on, switch the mode to something like xy, x, or y
        else if (plot_bm.tools.zoom.active) {
        	plot_bm.tools.zoom.current_mode = (plot_bm.tools.zoom.current_mode+1) % plot_bm.tools.zoom.modes.length;
        	$(this).attr('src','base/images/zoom_selected_'+plot_bm.tools.zoom.modes[plot_bm.tools.zoom.current_mode]+'.png');
        	
        	// Save the axis ranges between tools for continuity
			plot_bm.save_axis_ranges();
			
			plot_bm.replot();
        }
    });
    
    // When first building controls, select the button for the currently active tool (zoom / pan)
    if (plot_bm.tools.pan.active) {
		// Turn panning on
		$('#pan').addClass('selected').attr('src','base/images/pan_selected.png');
		plot_bm.tools.pan.active = true;
	}
    
    if (plot_bm.tools.zoom.active) {
		// Turn zooming on
		$('#zoom').addClass('selected').attr('src','base/images/zoom_selected_'+plot_bm.tools.zoom.modes[plot_bm.tools.zoom.current_mode]+'.png');
		plot_bm.tools.zoom.active = true;
	}
};

// Add interactivity to the plot through hovers and clicks.
plot_bm.assign_plot_actions = function() {
	var previousPoint = null;
	var previousAxis = null;
	
	// Behavior on window resize (redraw the plot)
	$(window).bind('resize', function() {
		plot_bm.replot();
	});

	// Behavior on hover (show a tooltip)
	divs.placeholder.bind("plothover", function (event, pos, item) {
		if (item) {
			if (previousPoint == null || previousPoint[0] != item.datapoint[0] || previousPoint[1] != item.datapoint[1]) {

				// If there was no axis highlighted, or the highlighted axis will be changing, remove the current highlighting (make sure the axis still exists first)
				if (previousAxis != null && previousAxis != item.series.id && plot_bm.axis_list[previousAxis] != undefined)
					plot_bm.axis_list[previousAxis].mouseout();
				
				// Acquire the current point and axis, assign them as the previous point and axis
				previousPoint = item.datapoint;
				previousAxis = item.series.id;

				// Remove the current tooltip if it exists
				$("#tooltip").remove();
				
				// Get the x and y coordinates of the current item
				x = item.datapoint[0].toFixed(2),
				y = item.datapoint[1].toFixed(2);
				
				// Trigger the hover event of the axis belonging to this item
				plot_bm.axis_list[item.series.id].mouseover();
				
				// If the tooltip option is on, show a tooltip for the item
				if ($("#enableTooltip:checked").length > 0) {
					showTooltip(item.pageX, item.pageY,
					item.series.label + ": (" + x + "," + y + ")");
				}
			}
		}
		else {
			// Clean up since we're not on an item. Remove the tooltip and axis hover (make sure the axis still exists first)
			if (previousAxis != null && plot_bm.axis_list[previousAxis] != undefined)
				plot_bm.axis_list[previousAxis].mouseout();
			$("#tooltip").remove();
			previousPoint = null;
		}
	});

	// Style and content of the hovering tooltip 
	function showTooltip(x, y, contents) {
		$('<div id="tooltip">' + contents + '</div>').css( {
			position: 'absolute',
			display: 'none',
			top: y + 5,
			left: x + 5,
			border: '1px solid #fdd',
			padding: '2px',
			'background-color': '#fee',
			opacity: 0.80
		}).appendTo("body").fadeIn(200);
	}	

	// Behavior on select (zoom in on the main plot)
	divs.placeholder.bind("plotselected", function (event, ranges) {		
		
		// Zoom in to the selection on the plot
		$.each(plot_bm.plot.getAxes(), function(axis) {
			if (this.used)
				plot_bm.options[this.direction+"axes"][this.n-1] = {min: ranges[axis].from, max: ranges[axis].to };
		});
		
		plot_bm.replot();

		// Don't zoom in on the overview
		plot_bm.overview.setSelection(ranges, true);
	});
	
	// Helper function: debug the plot by logging clicks
	/*
	divs.placeholder.bind("plotclick", function (event, pos, item) {
		if (item) {
			console.log("You clicked point " + item.dataIndex + " in " + item.series.label + ".");
			plot.highlight(item.series, item.datapoint);
		}
	});
	*/
};

// Assign actions for the plot options
plot_bm.assign_option_actions = function() {
	// Disable scroll zooming (useful on sensitive mice)
	$('#disableScrollZooming').click(function() {
		if ($(this).attr('checked'))
			plot_bm.options.zoom.interactive = false;
		else
			plot_bm.options.zoom.interactive = true;
		
		// Save the axis ranges (changing mode) and replot
		plot_bm.save_axis_ranges();
		plot_bm.replot();
	});
	
	// Click each of the default options by id
	$.each(plot_bm.default_options, function() {
		$('#'+this).click();
	});
};

// Helper function: save the currently viewed region for each plot axis (useful when switching modes)
plot_bm.save_axis_ranges = function() {

	// For all the used axes, set its max and min values to the current viewing region
	$.each(plot_bm.plot.getAxes(), function() {
		if (this.used) {
			plot_bm.options[this.direction+"axes"][this.n-1] = {min: this.min, max: this.max };
		}
	});
};

// Helper function: reset the plot axes to their default ranges
plot_bm.reset_axis_ranges = function() {
	if (plot_bm.plot) {
		// Reset the current list of x and y axes ranges
		plot_bm.options.xaxes = [];
		plot_bm.options.yaxes = [];
		
		// Reset the min and max values for each axis
		$.each(plot_bm.plot.getAxes(), function() {
			if (this.used)
				plot_bm.options[this.direction+"axes"][this.n-1] = { min: null, max: null};
		});
	}
};

// Add interactivity to the plot overview through selections and clicks.
plot_bm.assign_overview_actions = function() {
	// Behavior on select (zoom in on the main plot)
	divs.overview.bind("plotselected", function (event, ranges) {
		plot_bm.plot.setSelection(ranges);
	});

	// Behavior on click (zoom out to full view)
	divs.overview.bind("plotclick", function (event, pos, item) {
		plot_bm.plot = $.plot(divs.placeholder, plot_bm.active_data,
		$.extend(true, {}, plot_bm.options, {
			xaxis:{},
			yaxis:{}
		}));
	});
};
