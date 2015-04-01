$(document).ready(main);

function paragraph(text, start, end) {
  return '<p>' + text.substring(start, end) + '</p>';
}

function textToHtml(text) {
  var len = text.length;
  var nlCount = 0;
  var start = 0;
  var html = '';
  var i;

  for (i = 0; i < len; ++i) {
    if (text.charAt(i) == '\n')
      ++nlCount;
    else {
      if (nlCount > 1) {
        html += paragraph(text, start, i);
        start = i;
      }
      nlCount = 0;
    }
  }
  if (start < i)
    html += paragraph(text, start, i);

  return html;
}

function newItem(item) {
  var html = '<div class="item" id="item' + String(item.Id) + '" >';
  html += '<p class="summary">' + String(item.Summary) + '</p>';
  html += '<div class="details"><p>' + textToHtml(String(item.Details)) + '</p></div>';
  html += '<div class="hr"/>';
  html += '<span style="text-align: center"><a class="action" href="#">Upvote</a> | ' + String(item.Score) + '</span>';
  html += '</div>';
  return html;
}

function addItems(data) {
  var items = $('#items');
  var preloader = $('#preloader');
  for (var i = 0; i < data.length; ++i) {
    items.append(preloader, newItem(data[i]));
    items.append(preloader);
    $('#item' + data[i].Id).fadeIn();
  }
}

/*
function onPropose() {
  $('#proposeDialog').dialog({
    title: 'Propose an idea',
    modal: true,
    width: 700,
    height: 550,
    buttons: [
      {
        text: 'Submit',
        click: onSubmit
      }
    ]
  });
}
*/

function onSubmitSuccess(data, status, jqXHR) {
  $('#summary').val('');
  $('#details').val('');
  addItems([data]);
  $('#items').prepend($('#item' + data.Id));
}

function onSubmit() {
  var summary = $('#summary').val();
  var details = $('#details').val();

  $.post('/moderator/add/item', {Summary: summary, Details: details},
    onSubmitSuccess, 'json');
}

function main() {
  $('button').button();
  $.getJSON('/moderator/items', addItems);
  $('#preloader').hide();
}
