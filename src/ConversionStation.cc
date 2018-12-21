#include <set>
#include "ConversionStation.hh"
#include "InactiveElement.hh"
#include "MessageLogger.hh"

namespace material {

  /////////////////////////////////////////////////////
  /* CONVERSION STATION: 
   * VOLUME AT A SPECIFIC LOCATION, WICH PRODUCES CONVERSIONS.
   * CAN ALSO HAVE CONVERSION-INDEPENDENT LOCAL MATERIALS.
   *///////////////////////////////////////////////////

  const std::map<std::string, ConversionStation::Type> ConversionStation::typeString = {
    {"flange", FLANGE},
    {"second", SECOND}
  };


  void ConversionStation::build() {
    try {
      stationType_ = typeString.at(type_());
      buildConversions();
    } catch (const std::out_of_range& ex) {
      logERROR("Station type \"" + type_() + "\" not recognized.");
    }
    cleanup();
  }


  void ConversionStation::routeConvertedElements(MaterialObject& localOutput, MaterialObject& serviceOutput, InactiveElement& inactiveElement) {
    MaterialObject::Element* inputElement;
    //double totalGrams = 0.0;
    double multiplier = 0.0;
    bool converted = false;
    std::set<std::string> infoMaterials;
    std::set<std::string> warningMaterials;
    
    // CONVERSIONS
    for (const MaterialObject::Element* currElement : serviceElements_) { //inputElements) {
      converted = false;
      //if the material need to be converted (flange station, or endcap station with right destination)
      if ((stationType_ == FLANGE) || (stationType_ == SECOND && currElement->destination.state() && currElement->destination().compare(stationName_()) == 0)) {
        for (const Conversion* currConversion : conversions) {
          inputElement = currConversion->input->elements[0];
          if (inputElement->elementName().compare(currElement->elementName()) == 0) {
            converted = true;

            multiplier = currElement->quantityInUnit(inputElement->unit(), inactiveElement) / 
              inputElement->quantityInUnit(inputElement->unit(), inactiveElement);
          
            for (const MaterialObject::Element* outputElement : currConversion->outputs->elements) {
	      // newElement is the converted Element.
	      // newElement is assigned the properties from OUTPUT Element.
	      // Notably, in the rare case where outputElement has a componentName (see conversion station cfg file), 
	      // that name is assigned to newElement.
              MaterialObject::Element * newElement = new MaterialObject::Element(*outputElement, multiplier);

	      // Here, new Element is assigned additional info from the INPUT Elements.
	      // newElement is assigned the same componentName as in input
	      // (except if outputElement has a componentName).
	      if (currElement->componentName.state() && !outputElement->componentName.state()) {
		newElement->componentName(currElement->componentName());
	      }
	      // If we end up with a converted element belonging to no component, there is a problem !
	      if (!newElement->componentName.state()) {
		logWARNING("Element " + newElement->elementName() + "which is at output of station" + stationName_() + "has no assigned componentName.");
	      }
              if(currElement->debugInactivate()) {  //apply the inactivation also to converteds
                newElement->debugInactivate(true);
              }
              //TODO: check if is ok to do this
              if(currElement->destination.state() && !newElement->destination.state()) {  //apply the same destination of converted element (only if not defined in output conversion rule)
                newElement->destination(currElement->destination());
              }

	      // We now have our newElement :)
              if (newElement->service()) {
                if (newElement->unit().compare("g") != 0) {
                  serviceOutput.addElement(newElement);
                } else {
                  logERROR(err_service1 + newElement->elementName() + err_service2);
                }        
              } else {
                localOutput.addElement(newElement);
              }
            }
          }
        }
      }
      if (!converted) {
        serviceOutput.addElement(currElement);
        // We may want a warning or an info here
        bool doWarning = false;
        bool doInfo = false;
        if (stationType_ != SECOND) {
          // Always throw a warning for flange stations
          doWarning=true;
        } else {
          // it's a SECOND-level station: let's not overwarn
          // If I was the target of the failed conversion, then it's a warning, otherwise it's an info
          if (currElement->destination.state() && currElement->destination().compare(stationName_()) == 0) doWarning=true;
          else doInfo = true;
        }
        if (doWarning) warningMaterials.insert(currElement->elementName());
        if (doInfo) infoMaterials.insert(currElement->elementName());
      }
    }

    // WARNING IF ISSUES WHILE CONVERSION
    for(auto& warningMaterial : warningMaterials) {
      logWARNING("Element \"" + warningMaterial + "\" ignored by station \"" + stationName_() + "\".");
    }    
    for(auto& infoMaterial : infoMaterials) {
      logINFO("Element \"" + infoMaterial + "\" ignored by station \"" + stationName_() + "\".");
    }    


    // MATERIALS LOCAL TO THE STATIONS, INDEPENDENT FROM THE CONVERSIONS
    for (const std::unique_ptr<NonConvertedLocalMaterials>& it : nonConvertedLocalMaterials_) {
      for (const std::unique_ptr<MaterialObject::Element>& localElement : it->elements()) {

	// Local materials: if routed service type is found, there is a problem!
	if (localElement->service()) {
	  logWARNING("Element " + localElement->elementName() + ", which is local in station " + stationName_() + " is of routed service type!!!");
	}

	// Add the materials to the station volume
	localOutput.addElement(localElement.get());
      }
    }


  }


  /*
   * Build conversion station.
   * This stores all the station's conversions.
   * It also stores all the materials local to the station, and which are independent from the conversion.
   */
  void ConversionStation::buildConversions() {

    // Stores all conversions.
    for (auto& currentConversionNode : conversionsNode_) {
      Conversion* newConversion = new Conversion(subdetectorName_);
      newConversion->store(propertyTree());
      newConversion->store(currentConversionNode.second);
      newConversion->check();
      newConversion->build();

      conversions.push_back(newConversion);
    }

    // Stores conversion-independent info.
    for (const auto& node : nonConvertedLocalMaterialsNode_) {
      std::unique_ptr<NonConvertedLocalMaterials> newNonConverted (new NonConvertedLocalMaterials(subdetectorName_));
      newNonConverted->store(propertyTree());
      newNonConverted->store(node.second);
      newNonConverted->check();
      newNonConverted->build();

      nonConvertedLocalMaterials_.push_back(std::move(newNonConverted));
    }
  
  }



  /////////////////////////////////////////////////////
  /* CONVERSIONS: 
   * DEFINE THE ASSOCIATED OUTPUTS TO A GIVEN INPUT
   *///////////////////////////////////////////////////

  ConversionStation::Conversion::Conversion(const std::string subdetectorName) :
      inputNode_ ("Input", parsedAndChecked()),
      outputNode_ ("Output", parsedAndChecked()),
      subdetectorName_(subdetectorName)
    {};

 
  /*
   * Takes info from cfg files.
   */
  void ConversionStation::Conversion::build() {

    // Takes info from Input node.
    if (inputNode_.size() > 0) {
      input = new Inoutput(subdetectorName_);
      input->store(propertyTree());
      input->store(inputNode_.begin()->second);
      input->check();
      input->build();

      if(input->elements[0]->unit().compare("g") == 0) {
        logWARNING("Converted element unit is 'g' in conversion rule.");
      }
    }

    // Takes info from Output node.
    if (outputNode_.size() > 0) {
      outputs = new Inoutput(subdetectorName_);
      outputs->store(propertyTree());
      outputs->store(outputNode_.begin()->second);
      outputs->check();
      outputs->build();
    }
    cleanup();
  }



  ////////////////////////////////////////////////////////////////////
  /* INOUTPUTS: 
   * DEFINE A LIST OF ELEMENTS MAKING UP THE INPUT OR THE OUTPUT NODE
   *//////////////////////////////////////////////////////////////////

  ConversionStation::Inoutput::Inoutput(const std::string subdetectorName) :
    elementsNode_ ("Element", parsedOnly()),
    elementMaterialType(MaterialObject::Type::STATION),
    subdetectorName_(subdetectorName)
  {};


  /*
   * Takes info from cfg files.
   */
  void ConversionStation::Inoutput::build() {

    // Takes info from Element nodes.
    for (auto& currentElementNode : elementsNode_) {
      MaterialObject::Element* newElement = new MaterialObject::Element(elementMaterialType, subdetectorName_);
      newElement->store(propertyTree());
      newElement->store(currentElementNode.second);
      newElement->check();
      newElement->cleanup();

      elements.push_back(newElement);
    }
    cleanup();
  }



  /////////////////////////////////////////////////////
  /* LOCAL MATERIALS INDEPENDENT FROM THE CONVERSIONS.
   *///////////////////////////////////////////////////

  ConversionStation::NonConvertedLocalMaterials::NonConvertedLocalMaterials(const std::string subdetectorName) 
    : elementsNode_ ("Element", parsedOnly()),
      elementMaterialType_(MaterialObject::Type::STATION),
      subdetectorName_(subdetectorName)
  {};


  /*
   * Takes info from cfg files.
   */
  void ConversionStation::NonConvertedLocalMaterials::build() {

    // Takes info from Element nodes.
    for (const auto& currentElementNode : elementsNode_) {
      std::unique_ptr<MaterialObject::Element> newElement (new MaterialObject::Element(elementMaterialType_, subdetectorName_));
      newElement->store(propertyTree());
      newElement->store(currentElementNode.second);
      newElement->check();     
      newElement->cleanup();

      elements_.push_back(std::move(newElement));
    }
    cleanup();
  }
 

} /* namespace material */
