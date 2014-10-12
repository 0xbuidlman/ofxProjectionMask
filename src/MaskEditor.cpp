#include "MaskEditor.h"

const string designModeText = "Design";
const string designLiveModeText = "Design / Live";
const string liveModeText = "Live";
const string previewTagText = "Buffer preview";

const int bufferStartFrameNum = 120;

//Public
void MaskEditor::setMousePosition(int x, int y){
    mouseX = x;
    mouseY = y;
}

void MaskEditor::setPattern(BufferPattern *pattern){
    this->pattern = pattern;
    xml.assign(pattern->getBuffers());
}

void MaskEditor::setVolumes(float *playbackVolume, vector<float> *nonPlaybackVolumes){
    textArea.setVolumes(playbackVolume, nonPlaybackVolumes);
}

void MaskEditor::update(){
    if(!isTransforming()){
        objectStore.updateHighlights(mouseX, mouseY);
    }
    
    if(mode == Design){
        textArea.setRenderMode(designModeText);
    }else if(mode == DesignLive){
        textArea.setRenderMode(designLiveModeText);
    }else if(mode == Live){
        textArea.setRenderMode(liveModeText);
    }
}

Canvas *MaskEditor::getLiveCanvas(){
    return &this->liveCanvas;
}

void MaskEditor::draw(){
    
    //drawBufferPreviews();
    
    ofPushMatrix();
    ofTranslate(designCanvas.getX(), designCanvas.getY());
    designCanvas.draw();
    objectStore.drawDesign();
    ofPopMatrix();
    
    ofPushMatrix();
    ofTranslate(liveCanvas.getX(), liveCanvas.getY());
    if(mode == Design){
        liveCanvas.draw();
    }
    objectStore.drawLive(mode);
    ofPopMatrix();
    
    textArea.draw();
    
    drawLiveCursor();
}

void MaskEditor::undo(){
    objectStore.undo();
}

void MaskEditor::redo(){
    objectStore.redo();
}

void MaskEditor::loadObjects(){
    this->xml.load();
}

void MaskEditor::saveObjects(){
    this->xml.save();
}

void MaskEditor::autoSaveObjects(){
    this->xml.save(true);
}

void MaskEditor::respondToMouseDrag(){
    if(selectedMaskFrame != 0){
        TransformState transformState = selectedMaskFrame->getTransformState();
        if(transformState == Translating){
            selectedMaskFrame->setPosition(mouseX - mouseOffset.x, mouseY - mouseOffset.y);
        }else if(transformState == Scaling){
            scaleSelectedMaskFrame();
        }else if(transformState == Masking){
            selectedMaskFrame->setSelectedMaskPointPosition(mouseX - mouseOffset.x, mouseY - mouseOffset.y);
        }
    }
}

void MaskEditor::respondToMouseDown(){
    selectedMaskFrame = objectStore.beginTransform();
    if(selectedMaskFrame != 0){
        setMouseOffsetFromSelectedMaskFrame();
    }
}

void MaskEditor::respondToMouseRelease(){
    selectedMaskFrame = 0;
    objectStore.endTransform();
    autoSaveObjects();
}

void MaskEditor::respondToKey(int key){
    if(key == 'f' || key == 'F'){
        this->createNewMaskFrame();
        autoSaveObjects();
    }else if(key == 'p' || key == 'P'){
        this->createNewMaskPoint();
        autoSaveObjects();
    }else if (key == ' '){
        ofToggleFullscreen();
    }else if(key == 'u' || key == 'U'){
        this->undo();
    }else if(key == 'r' || key == 'R'){
        this->redo();
    }else if(key == 127 || key == 8){
        this->deleteHighlightedItem();
        autoSaveObjects();
    }else if(key == 356){
        this->nudge(Left);
        autoSaveObjects();
    }else if(key == 357){
        this->nudge(Up);
        autoSaveObjects();
    }else if(key == 358){
        this->nudge(Right);
        autoSaveObjects();
    }else if(key == 359){
        this->nudge(Down);
        autoSaveObjects();
    }else if(key == 's' || key == 'S'){
        this->saveObjects();
    }else if(key == 'l' || key == 'L'){
        this->loadObjects();
    }else if(key == 'm' || key == 'M'){
        this->cycleMode();
    }else if(key == 'e' || key == 'E'){
        this->toggleFrameNudge();
    }
}

//Protected
void MaskEditor::initialise(){
    
    ofSetHexColor(0xFFFFFF);
	ofBackground(0, 0, 0);
    ofSetWindowPosition(0, 0);
    ofSetWindowShape(presets.windowWidth, presets.windowHeight);
    ofEnableAlphaBlending();
    
    designCanvas.setPosition(presets.designCanvasX, presets.designCanvasY);
    designCanvas.setSize(presets.designCanvasWidth, presets.designCanvasHeight);
    designCanvas.setNumGridLines(presets.numGridLinesX, presets.numGridLinesY);
    
    liveCanvas.setPosition(presets.liveCanvasX, presets.liveCanvasY);
    liveCanvas.setSize(presets.liveCanvasWidth, presets.liveCanvasHeight);
    liveCanvas.setNumGridLines(presets.numGridLinesX, presets.numGridLinesY);
    
    textArea.setInstructionsPosition(presets.instructionsX, presets.instructionsY);
    textArea.setPlaybackVolumePosition(presets.playbackVolumeX, presets.playbackVolumeY);
    textArea.setNonPlaybackVolumesPosition(presets.nonPlaybackVolumeX, presets.nonPlaybackVolumeY);
    textArea.setNumberBoxSize(presets.numberBoxWidth, presets.numberBoxHeight);
    textArea.setOffsets(presets.numberTagOffsetX, presets.numberTagOffsetY);
    textArea.setMargins(presets.numberTagMargin, presets.numberBoxMargin);
    
    ofSetFullscreen(presets.startFullscreen);
    
    selectedMaskFrame = 0;
    
    xml.assign(&designCanvas, &liveCanvas, &objectStore);
}

void MaskEditor::nudge(Direction direction){
    objectStore.nudge(direction);
}

void MaskEditor::toggleFrameNudge(){
    objectStore.toggleFrameNudge();
    textArea.setFrameNudgeEnabled(objectStore.getFrameNudgeEnabled());
}

void MaskEditor::createNewMaskFrame(){
    MaskFrame maskFrame;
    maskFrame.assignCanvases(designCanvas, liveCanvas);
    maskFrame.setBuffers(pattern->getBuffers());
    maskFrame.setSize(presets.newMaskFrameWidth, presets.newMaskFrameHeight);
    maskFrame.setPosition(mouseX, mouseY);
    objectStore.add(&maskFrame);
}

void MaskEditor::createNewMaskPoint(){
    objectStore.createMaskPointAt(mouseX, mouseY);
}

void MaskEditor::deleteHighlightedItem(){
    objectStore.erase();
}

void MaskEditor::cycleMode(){
    if(mode == Design){
        mode = DesignLive;
    }else if(mode == DesignLive){
        mode = Live;
    }else if(mode == Live){
        mode = Design;
    }
}

bool MaskEditor::mouseIsOverDesignCanvas(){
    return mouseX > this->designCanvas.getX() && mouseX < (this->designCanvas.getX() + this->designCanvas.getWidth()) 
        && mouseY > this->designCanvas.getY() && mouseY < (this->designCanvas.getY() + this->designCanvas.getHeight());
}

void MaskEditor::drawLiveCursor(){
    if(mouseIsOverDesignCanvas() && mode != Live){
        int liveMouseX = ofMap(mouseX, designCanvas.getX(), designCanvas.getX() + designCanvas.getWidth(), liveCanvas.getX(), liveCanvas.getX() + liveCanvas.getWidth());
        int liveMouseY = ofMap(mouseY, designCanvas.getY(), designCanvas.getY() + designCanvas.getHeight(), liveCanvas.getY(), liveCanvas.getY() + liveCanvas.getHeight());
        
        ofLine(liveMouseX - 5, liveMouseY - 5, liveMouseX + 5, liveMouseY + 5);
        ofLine(liveMouseX + 5, liveMouseY - 5, liveMouseX - 5, liveMouseY + 5);
    }
}

void MaskEditor::drawBufferPreviews(){
    ofPushMatrix();
    ofTranslate(presets.bufferPreviewX, presets.bufferPreviewY);
    
    ofSetColor(255, 255, 255, 255);
    ofNoFill();
    
    int x = 0, y = 0, count = 0;
    vector<ofFbo>* buffers = pattern->getBuffers();
    
    for(int i = 0; i < buffers->size(); i++){
        ofTranslate(x, y);
        if(i > 0 && i % presets.stackAfter == 0){
            ofPopMatrix();
            ofPushMatrix();
            count++;
            ofTranslate(presets.bufferPreviewX + ((presets.bufferPreviewWidth + presets.bufferMargin) * count), presets.bufferPreviewY);
        }
        y = presets.bufferPreviewHeight + (presets.bufferMargin * 2);
        if(ofGetFrameNum() > bufferStartFrameNum){
            buffers->at(i).draw(0, 0, presets.bufferPreviewWidth, presets.bufferPreviewHeight);
        }
        ofRect(0, 0, presets.bufferPreviewWidth, presets.bufferPreviewHeight);
        ofDrawBitmapString("Buffer " + ofToString(i + 1), 0, presets.bufferPreviewHeight + presets.bufferMargin);
    }
    
    ofPopMatrix();
}

bool MaskEditor::isTransforming(){
    return selectedMaskFrame != 0 && selectedMaskFrame->getTransformState() != NoTransform;
}

void MaskEditor::scaleSelectedMaskFrame(){
    int mouseXConstrained = clampInt(mouseX, designCanvas.getX() + mouseOffset.x, designCanvas.getX() + designCanvas.getWidth() + mouseOffset.x);
    int mouseYConstrained = clampInt(mouseY, designCanvas.getY() + mouseOffset.y, designCanvas.getY() + designCanvas.getHeight() + mouseOffset.y);
    int newWidth, newHeight;
    
    if(selectedCorner == TopLeft){
        
        newWidth = (selectedMaskFrame->getX() + selectedMaskFrame->getWidth() + mouseOffset.x) - mouseXConstrained;
        newHeight = (selectedMaskFrame->getY() + selectedMaskFrame->getHeight() + mouseOffset.y) - mouseYConstrained;
        
    }else if(selectedCorner == TopRight){
        
        newWidth = mouseXConstrained - (selectedMaskFrame->getX() + mouseOffset.x);
        newHeight = (selectedMaskFrame->getY() + selectedMaskFrame->getHeight() + mouseOffset.y) - mouseYConstrained;
        
    }else if(selectedCorner == BottomRight){
        
        newWidth = mouseXConstrained - (selectedMaskFrame->getX() + mouseOffset.x);
        newHeight = mouseYConstrained - (selectedMaskFrame->getY() + mouseOffset.y);
        
    }else if(selectedCorner == BottomLeft){
        
        newWidth = (selectedMaskFrame->getX() + selectedMaskFrame->getWidth() + mouseOffset.x) - mouseXConstrained;
        newHeight = mouseYConstrained - (selectedMaskFrame->getY() + mouseOffset.y);
    }
    
    int smallestLegalWidth = selectedMaskFrame->getSmallestLegalWidth(selectedCorner);
    int smallestLegalHeight = selectedMaskFrame->getSmallestLegalHeight(selectedCorner);
    int newWidthClamped = clampInt(newWidth, smallestLegalWidth);
    int newHeightClamped = clampInt(newHeight, smallestLegalHeight);
    selectedMaskFrame->setSize(newWidthClamped, newHeightClamped, selectedCorner);
}

void MaskEditor::setMouseOffsetFromSelectedMaskFrame(){
    TransformState transformState = selectedMaskFrame->getTransformState();
    if(transformState == Translating){
        setMouseOffsetFromTopLeftCorner();
    }else if(transformState == Scaling){
        setMouseOffsetFromSelectedCorner();
    }else if(transformState == Masking){
        setMouseOffsetFromSelectedMaskPoint();
    }
}

void MaskEditor::setMouseOffsetFromSelectedMaskPoint(){
    mouseOffset.x = mouseX - selectedMaskFrame->getSelectedMaskPointX();
    mouseOffset.y = mouseY - selectedMaskFrame->getSelectedMaskPointY();
}

void MaskEditor::setMouseOffsetFromSelectedCorner(){
    selectedCorner = selectedMaskFrame->highlightedCorner();
    if(selectedCorner == TopLeft){
        setMouseOffsetFromTopLeftCorner();
    }else if(selectedCorner == TopRight){
        setMouseOffsetFromTopRightCorner();
    }else if(selectedCorner == BottomRight){
        setMouseOffsetFromBottomRightCorner();
    }else if(selectedCorner == BottomLeft){
        setMouseOffsetFromBottomLeftCorner();
    }
}

void MaskEditor::setMouseOffsetFromTopLeftCorner(){
    mouseOffset.x = mouseX - selectedMaskFrame->getX();
    mouseOffset.y = mouseY - selectedMaskFrame->getY();
}

void MaskEditor::setMouseOffsetFromTopRightCorner(){
    mouseOffset.x = mouseX - selectedMaskFrame->getX() - selectedMaskFrame->getWidth();
    mouseOffset.y = mouseY - selectedMaskFrame->getY();
}

void MaskEditor::setMouseOffsetFromBottomRightCorner(){
    mouseOffset.x = mouseX - selectedMaskFrame->getX() - selectedMaskFrame->getWidth();
    mouseOffset.y = mouseY - selectedMaskFrame->getY() - selectedMaskFrame->getHeight();
}

void MaskEditor::setMouseOffsetFromBottomLeftCorner(){
    mouseOffset.x = mouseX - selectedMaskFrame->getX();
    mouseOffset.y = mouseY - selectedMaskFrame->getY() - selectedMaskFrame->getHeight();
}