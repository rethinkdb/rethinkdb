// I know I'm writing all this things in window, but it's a testing tool

Handlebars.registerHelper('print_safe', function(str) {
    if (str != null) {
        return new Handlebars.SafeString(str);
    }
    else {
        return "";
    }
});

template_json_tree = {
    'container': Handlebars.compile($('#dataexplorer_result_json_tree_container-template').html()),
    'span': Handlebars.compile($('#dataexplorer_result_json_tree_span-template').html()),
    'span_with_quotes': Handlebars.compile($('#dataexplorer_result_json_tree_span_with_quotes-template').html()),
    'url': Handlebars.compile($('#dataexplorer_result_json_tree_url-template').html()),
    'email': Handlebars.compile($('#dataexplorer_result_json_tree_email-template').html()),
    'object': Handlebars.compile($('#dataexplorer_result_json_tree_object-template').html()),
    'array': Handlebars.compile($('#dataexplorer_result_json_tree_array-template').html())
};


json_to_node = function(value) {
    var data, element, key, last_key, output, sub_values, value_type, _i, _len;
    value_type = typeof value;
    output = '';
    if (value === null) {
        return template_json_tree.span({
            classname: 'jt_null',
            value: 'null'
        });
    }
    else if ((value.constructor != null) && value.constructor === Array) {
        if (value.length === 0) {
            return '[ ]';
        }
        else {
            sub_values = [];
            for (_i = 0, _len = value.length; _i < _len; _i++) {
                element = value[_i];
                sub_values.push({
                    value: this.json_to_node(element)
                });
                if (typeof element === 'string' && (/^(http|https):\/\/[^\s]+$/i.test(element) || /^[a-z0-9._-]+@[a-z0-9]+.[a-z0-9._-]{2,4}/i.test(element))) {
                    sub_values[sub_values.length - 1]['no_comma'] = true;
                }
            }
            sub_values[sub_values.length - 1]['no_comma'] = true;
            return template_json_tree.array({
                values: sub_values
            });
        }
    }
    else if (value_type === 'object') {
        sub_values = [];
        for (key in value) {
            last_key = key;
            sub_values.push({
                key: key,
                value: this.json_to_node(value[key])
            });
            if (typeof value[key] === 'string' && (/^(http|https):\/\/[^\s]+$/i.test(value[key]) || /^[a-z0-9._-]+@[a-z0-9]+.[a-z0-9._-]{2,4}/i.test(value[key]))) {
                sub_values[sub_values.length - 1]['no_comma'] = true;
            }
        }
        if (sub_values.length !== 0) {
            sub_values[sub_values.length - 1]['no_comma'] = true;
        }
        data = {
        no_values: false,
        values: sub_values
        };
        if (sub_values.length === 0) {
            data.no_value = true;
        }
        return template_json_tree.object(data);
    }
    else if (value_type === 'number') {
        return template_json_tree.span({
            classname: 'jt_num',
            value: value
        });
    }
    else if (value_type === 'string') {
        if (/^(http|https):\/\/[^\s]+$/i.test(value)) {
            return template_json_tree.url({
                url: value
            });
        }
        else if (/^[a-z0-9]+@[a-z0-9]+.[a-z0-9]{2,4}/i.test(value)) {
            return template_json_tree.email({
                email: value
            });
        }
        else {
            return template_json_tree.span_with_quotes({
                classname: 'jt_string',
                value: value
            });
        }
    }
    else if (value_type === 'boolean') {
        return template_json_tree.span({
            classname: 'jt_bool',
            value: value ? 'true' : 'false'
        });
    }
}


$(document).ready( function() {

    $.ajax({
        contentType: 'application/json',
        url: '/get_semilattice',
        success: function(response) {
            $('#semilattice_raw').html(response);

            $('#semilattice_raw').height(0);
            height = $('#semilattice_raw').prop('scrollHeight')+30;
            $('#semilattice_raw').css('height', height);
            
            $('#nice_representation').html(json_to_node(JSON.parse(response)));
        }
    });

    $('.submit_data').click(function() {
        data = JSON.stringify(JSON.parse($('#semilattice_raw').val()), null, 2);
        $.ajax({
            contentType: 'application/json',
            url: '/submit_semilattice',
            type: 'POST',
            data: data,
            success: function(response) {
                console.log(response)
            }
        });
    });

    $('.refresh').click(function() {
        // Ultimate lazyness
        window.location.reload();
    });



});
