var plot_bm = {
	data: {},
	default_sets: ['edb-ssd','myisam-ssd'],
	default_series: ['time'],
	colors: {
		"EDB on SSD" : "rgb(204,0,51)",
		"EDB on HDD" : "rgb(255,204,0)",
		"MyISAM on SSD" : "rgb(0,51,255)",
		"MyISAM on HDD" : "rgb(112,224,0)"
	},
	filename_date_format: 'yyyyMMdd-HHmmss',
	formatted_date: 'MMM d, yyyy - h:mm tt',
	date_latest_filename: 'latest.js',
	date_latest: 'Latest',
	all_data: '*',
	current_script: '',
	current_test: '',
	current_date: ''	
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
			shadowSize: 6,
		},
		grid: {
			clickable: true,
			hoverable: true
		},
		lines: { show: true },
		points: { show: true },
		selection: { mode: "xy" }
	};
	
	plot_bm.build_controls();
	plot_bm.assign_plot_actions();
	plot_bm.assign_overview_actions();
	
	/* Determine which file we are being asked to load */
	get_data(window.location.hash.slice(1));
});

$(window).load(function() {
	plot_bm.replot();
});
// Replot the current plot
plot_bm.replot = function() {
	plot_bm.active_data = [];
	var datetime_format = '';
	if (plot_bm.current_date != '')
		datetime_format = 'Benchmarks generated on '+Date.parseExact(plot_bm.current_date, plot_bm.filename_date_format).toString(plot_bm.formatted_date)+'.';

	// Collect the active data based on the user's selections
	$('.set:checked').each(function(key,val) {
		set = $(this).attr('name');
		var y_axis_num = 1;
		$('.series:checked').each(function (key, val) {
			series = $(this).attr('name');
			if (plot_bm.data[set][series] != null) {
				plot_bm.active_data.push({
					label : set + " : " + series,
					data : plot_bm.data[set][series],
					yaxis: y_axis_num
				});
			y_axis_num = y_axis_num + 1;
			}
		});
	});


	// Update the date/timestamp
	divs.time.text(datetime_format);

	// Make the plot's height fill the window
	var h = $(window).height() - divs.header.outerHeight() - divs.footer.outerHeight() - divs.placeholder.css('margin-top').replace('px','') - divs.placeholder.css('margin-bottom').replace('px','');
	divs.placeholder.height(h);
	
	// Draw the plot
	plot_bm.plot = $.plot(divs.placeholder, plot_bm.active_data, {
		lines: { show: true },
		points: { show: true },
		selection: { mode: "xy" },
		grid: { hoverable: true, clickable: true },
	});
	
	// Draw the plot overview
	plot_bm.overview = $.plot(divs.overview, plot_bm.active_data, {
		legend: { show: false },
		lines: { show: true, lineWidth: 1 },
		shadowSize: 0,
		xaxis: { ticks: 4 },
		yaxis: { ticks: 3 },
		grid: { color: "#999", clickable: true },
		selection: { mode: "xy" }
	});
};

// Build the controls for the plot
plot_bm.build_controls = function() {
	var data_set_labels = [];
	var data_series_labels = [];
	
	$('#chooseSets').empty();
	$('#chooseSeries').empty();

	// Create the checkboxes
	$.each(plot_bm.data, function(key, set) {
		//series.color = colors[series.label];
		$('<p class="controlOption"><input class="set" name="'+key+'" type="checkbox">'+key+'</input></p>').appendTo('#chooseSets');
		
		data_set_labels.push(key);
			
		$.each(set, function(key, series) {
			if ($.inArray(key,data_series_labels) < 0 ) {
				$('<p class="controlOption"><input class="series" name="'+key+'" type="checkbox">'+key+'</input></p>').appendTo('#chooseSeries');
				data_series_labels.push(key);
			}
		});
	});
	
	// Make sure the default series and default sets are checked
	$.each(plot_bm.default_sets, function() {
		$('.set[name^='+this+']').attr('checked','true');
	});
	$.each(plot_bm.default_series, function() {
		$('.series[name^='+this+']').attr('checked','true');
	});

	// Update the plot whenever a set is selected or deselected
	$('.set').click(function() {
		plot_bm.replot();
	}); 

	// Update the plot whenever a series is selected or deselected. Allow up to two selections (one on each y-axis).
	$('.series').click(function() {
		if ($(this).attr('checked')) {
			if ($('.series:checked').length == 2)
				$('.series').not(':checked').attr('disabled','disabled');
		}
		else 
			$('.series:disabled').removeAttr('disabled');

		plot_bm.replot();
	});

	$('#chooseSetsHeader').jcollapser({target: '#chooseSets', state: 'collapsed'});	
	$('#chooseSeriesHeader').jcollapser({target: '#chooseSeries', state: 'expanded'});	
	$('#plotOptionsHeader').jcollapser({target: '#plotOptions', state: 'collapsed'});	
	$('#overviewHeader').jcollapser({target: '#overview', state: 'collapsed'});	
};

var get_data = function(url) {
	$('#loadingAnimation').remove();
	$('#placeholder').animate({opacity: 0.2}, 1000);
	$('<img id="loadingAnimation" src="base/images/loading-animation.gif" />').appendTo("#plot");
	$.getJSON(url, function(data) {
	    plot_bm.data = data;
        plot_bm.build_controls();
        $('#loadingAnimation').animate( {opacity: 0.0}, 1000, function() {
            $(this).remove();
            $('#placeholder').animate({opacity: 1.0}, 1000);

            var plot_title = $('#plotTitle').text(url);

            plot_bm.replot();
        });
    });
};
// Add interactivity to the plot through hovers and clicks.
plot_bm.assign_plot_actions = function() {
	var previousPoint = null;
	
	// Behavior on window resize (redraw the plot)
	$(window).bind('resize', function() {
		plot_bm.replot();
	});

	// Behavior on hover (show a tooltip)
	divs.placeholder.bind("plothover", function (event, pos, item) {
		if ($("#enableTooltip:checked").length > 0) {
			if (item) {
				if (previousPoint == null || previousPoint[0] != item.datapoint[0] || previousPoint[1] != item.datapoint[1]) {

					previousPoint = item.datapoint;

					$("#tooltip").remove();
					x = item.datapoint[0].toFixed(2),
					y = item.datapoint[1].toFixed(2);

					showTooltip(item.pageX, item.pageY,
					item.series.label + ": (" + x + "," + y + ")");
				}
			}
			else {
				$("#tooltip").remove();
				previousPoint = null;
			}
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

	// Behavior on click
	/*
	divs.placeholder.bind("plotclick", function (event, pos, item) {
		if (item) {
			alert("You clicked point " + item.dataIndex + " in " + item.series.label + ".");
			plot.highlight(item.series, item.datapoint);
		}
	});
	*/

	// Behavior on select (zoom in on the main plot)
	divs.placeholder.bind("plotselected", function (event, ranges) {
		// Zoom in to the selection on the plot
		plot_bm.plot = $.plot(divs.placeholder, plot_bm.active_data,
		$.extend(true, {}, plot_bm.options, {
			xaxis: { min: ranges.xaxis.from, max: ranges.xaxis.to },
			yaxis: { min: ranges.yaxis.from, max: ranges.yaxis.to }
		}));

		// Don't zoom in on the overview to avoid the loop
		plot_bm.overview.setSelection(ranges, true);
	});
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
