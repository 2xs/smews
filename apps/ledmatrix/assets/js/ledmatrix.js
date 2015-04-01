var picker_is_shown = false;
function updateColor()
{
    if (! picker_is_shown)
    {
        $.ajax({
            type: "GET",
            url: "led_get_color",
            success: function (responseData) {
                $("#colorinput").spectrum({
                    color: "#" + responseData,
                    show: function() {picker_is_shown = true;},
                    hide: function() {picker_is_shown = false;}
                });
            }
        });
        setTimeout(updateColor,2000);
    }
}
function updateText()
{
    if (! $("#textinput").is(":focus"))
    {
        $.ajax({
            type: "GET",
            url: "led_get_text",
            success: function (responseData) {
                $("#textinput").val(responseData);
            }
        });
        setTimeout(updateText,2000);
    }
        
}
function changeColor(color)
{
    $.ajax({
        type: "GET",
        url: "led_color?color="+color.substring(1),
        complete: updateColor,
    });
}

function changeText(text)
{
    $.ajax({
        type: "GET",
        url: "led_text?text="+text,
        complete: updateText,
    });
    
}

$(document).ready(function() {
        
    /* When document is ready, get color and text from server */
    updateColor();
    updateText();
});
