function changeColor(color)
{
    $.ajax({
        type: "GET",
        url: "led_color?color="+color.substring(1),
    });
}

function changeText(text)
{
    $.ajax({
        type: "GET",
        url: "led_text?text="+text,
    });
    
}

$(document).ready(function() {
    /* When document is ready, get color and text from server */
    $.ajax({
        type: "GET",
        url: "led_get_text",
        success: function (responseData) {
            console.log(responseData);
            $("#textinput").val(responseData);
        }
    }
          );
    $.ajax({
        type: "GET",
        url: "led_get_color",
        success: function (responseData) {
            console.log(responseData);
            $("#colorinput").spectrum({color: "#" + responseData});
        }
    });
});
