function bt_obj_get( vis_obj_id )
{
  return document.getElementById( vis_obj_id );
}

// ************************************************************************** //

function bt_obj_remove( vis_obj )
{
  if( !vis_obj ) return;

  vis_obj.style.display = "none";
}

// ************************************************************************** //

function bt_obj_display( vis_obj )
{
  if( !vis_obj ) return;

  vis_obj.style.display = "block";
}

// ************************************************************************** //

function bt_is_displayed( vis_obj )
{
  return vis_obj.style.display == "block";
}

// ************************************************************************** //

function bt_obj_toggle( vis_obj )
{
  if( !vis_obj ) return;

  if( bt_is_displayed( vis_obj ) ) {
    bt_obj_remove( vis_obj );
  }
  else {
    bt_obj_display( vis_obj ); 
  }
}

// ************************************************************************** //

function bt_set_html( vis_obj, html ) {
    if( !vis_obj ) return;

    html = (html != null && html != undefined) ? html : '';

    vis_obj.innerHTML = html;
}

// ************************************************************************** //

function toggle_element( targ_id, toggle_id, on_label, off_label )
{
  var targ   = bt_obj_get( targ_id )
  var toggle = bt_obj_get( toggle_id )

  if( bt_is_displayed( targ ) ) {
    bt_obj_remove( targ );
    if( toggle )
      bt_set_html( toggle, on_label );
  }
  else {
    bt_obj_display( targ ); 
    if( toggle )
      bt_set_html( toggle, off_label );
  }
}

// ************************************************************************** //

function select_form_page( next_page_id, curr_page_id ) 
{
  if( curr_page_id != next_page_id ) {
    bt_obj_remove ( bt_obj_get( curr_page_id ) );
    bt_obj_display( bt_obj_get( next_page_id ) ); 
    curr_page_id = next_page_id;
  }

  return curr_page_id;
}

// ************************************************************************** //

// EOF
