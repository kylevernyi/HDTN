function Disk(paramSvgRootGroup, paramX, paramY, paramRadius, paramPerspectiveRadius, paramHeight, paramDiskName) {

    var svgRootGroup = paramSvgRootGroup;
    var x = paramX;
    var y = paramY;
    var radius = paramRadius;
    var perspectiveRadius = paramPerspectiveRadius;
    var height = paramHeight;
    var diskName = paramDiskName;

    var diskGroup = svgRootGroup.append("svg:g")
        .attr("transform", "translate(" + x + "," + y +")")
        .attr("class", "disk_node_group");

    function GetCylinderFillPath(rx, ry, height) {
        return "M" + (-rx) + ",0"
            + "a" + rx + "," + ry + " 0 1 0 " + (2 * rx) + ",0" //arc bottom left to bottom right
            + "l 0," + (-height)
            + "a" + rx + "," + ry + " 0 1 0 " + (2 * -rx) + ",0" //arc top right to top left
            + "z";
    }
    function GetCylinderDashedLinePath(rx, ry) {
        return "M" + (-rx) + ",0"
            + "a" + rx + "," + ry + " 0 1 1 " + (2 * rx) + ",0"; //arc bottom left to bottom right
    }
    function GetCylinderTopFrontLinePath(rx, ry, height) {
        return "M" + (-rx) + "," + (-height)
            + "a" + rx + "," + ry + " 0 1 0 " + (2 * rx) + ",0"; //arc top left to top right
    }

    function SetUsage(percentage, bufferPercentage) {
        diskGroup.selectAll("path.cylinder_top_front_line_path")
            .attr("d", GetCylinderTopFrontLinePath(radius, perspectiveRadius, height*percentage*.01));


        diskGroup.selectAll("path.cylinder_fill_path")
            .attr("d", GetCylinderFillPath(radius, perspectiveRadius, height*percentage*.01));

        diskGroup.selectAll("text.cylinder_text")
            .text(percentage.toFixed(0) + "%");

        diskGroup.selectAll("stop")
            .attr('offset', bufferPercentage*.01);

    }

    function GetRightWirePosition() {
        return {
            x: x + radius,
            y: y - height * .5
        };
    }
    function GetLeftWirePosition() {
        return {
            x: x - radius,
            y: y - height * .5
        };
    }
    function GetBottomWirePosition() {
        return {
            x: x,
            y: y
        };
    }
    function GetTopWirePosition() {
        return {
            x: x,
            y: y - (height + perspectiveRadius)
        };
    }



    //DRAW THE HIDRA NODE
    diskGroup.append("svg:path")
        .attr("d", GetCylinderFillPath(radius, perspectiveRadius, height))
        .attr("stroke", "black")
        .attr("fill", "none");

    diskGroup.append("svg:path")
        .attr("d", GetCylinderDashedLinePath(radius, perspectiveRadius))
        .attr("stroke", "black")
        .attr("fill", "none")
        .attr("stroke-dasharray", "4 2");

    diskGroup.append("svg:path")
        .attr("d", GetCylinderTopFrontLinePath(radius, perspectiveRadius, height))
        .attr("stroke", "black")
        .attr("fill", "none");

    diskGroup.append("svg:path")
        .attr("class", "cylinder_top_front_line_path")
        .attr("d", GetCylinderTopFrontLinePath(radius, perspectiveRadius, height*.5))
        .attr("stroke", "black")
        .attr("fill", "none");

    diskGroup.append("svg:path")
        .attr("class", "cylinder_fill_path")
        .attr("d", GetCylinderFillPath(radius, perspectiveRadius, height*.5))
        .attr("stroke", "none")
        .attr("fill", "red")
        .style("opacity", .2);

    diskGroup.append("svg:path")
        .attr("class", "cylinder_fill_path")
        .attr("d", GetCylinderFillPath(radius, perspectiveRadius, height*.5))
        .attr("stroke", "black")
        .attr("fill", "none");

    diskGroup.append("svg:text")
        .attr("class", "cylinder_text")
        .attr("dy", ".35em")
        //.attr("text-anchor", "end")
        .attr("transform", "translate(0," + (-height*.5) +")")
        .attr("text-anchor", "middle")
        .text("");

    diskGroup.append("svg:text")
        .attr("class", "disk_name_text")
        .attr("dy", ".35em")
        //.attr("text-anchor", "end")
        .attr("transform", "translate(0," + (30) +")")
        .attr("text-anchor", "middle")
        .text(diskName);



    var bufferGradient = diskGroup.append('linearGradient')
        .attr('id', "gradient_" + diskName)
        .attr("x1", "0%")
        .attr("y1", "100%")
        .attr("x2", "0%")
        .attr("y2", "0%");
    bufferGradient.append('stop')
        .attr("class", "buffer_fill_color");
        //.attr('stop-color', 'lawngreen');
    bufferGradient.append('stop')
        .attr("class", "buffer_nonfill_color");
        //.attr('stop-color', 'white');
    bufferGradient.selectAll("stop")
        .attr('offset', function(d) {
            return .2;
        });



    diskGroup.append("svg:rect")
        .attr("x", radius + 10)
        .attr("y", -height)
        .attr("width", 10)
        .attr("height", height)
        .attr("class", "buffer_rect")
        .attr("rx", 2)
        .attr("ry", 2)
        .attr("fill", "url(#gradient_" + diskName + ")")
        .style("stroke-width", 1)
        .style("stroke", "black");



    return {
        SetDiskUsage: function(percentage, bufferPercentage){
            SetUsage(percentage, bufferPercentage);
        },
        GetWirePos: function(posStr) {
            if(posStr === "right") return GetRightWirePosition();
            if(posStr === "left") return GetLeftWirePosition();
            if(posStr === "top") return GetTopWirePosition();
            if(posStr === "bottom") return GetBottomWirePosition();
        }
    };
}
