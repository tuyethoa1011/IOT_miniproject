function update(){
    $.get("/update_humid", function(data){
                $("#h_output").html(data)
        });
    $.get("/update_temperature", function(data){
                $("#t_output").html(data)
        });
    $.get("/update_rain", function(data){
                $("#r_output").html(data)
        });
}
    
update()

var intervalId = setInterval(function() {
            update()
}, 100);